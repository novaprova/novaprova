#include "common.h"
#include <sys/wait.h>
#include <sys/poll.h>
#include "u4c_priv.h"
#include "except.h"
#include <valgrind/memcheck.h>

__u4c_exceptstate_t __u4c_exceptstate;
static u4c_globalstate_t *state;

void
__u4c_begin(u4c_globalstate_t *s)
{
    state = s;
}

void
__u4c_end(void)
{
    state = 0;
}

static const u4c_event_t *
normalise_event(const u4c_event_t *ev)
{
    static u4c_event_t norm;

    memset(&norm, 0, sizeof(norm));
    norm.which = ev->which;
    norm.description = xstr(ev->description);
    if (ev->lineno == ~0U)
    {
	unsigned long pc = (unsigned long)ev->filename;
	static char pcbuf[32];

	if (!__u4c_describe_address(state, pc,
				    &norm.filename,
				    &norm.lineno,
				    &norm.function))
	{
	    snprintf(pcbuf, sizeof(pcbuf), "(0x%lx)", pc);
	    norm.function = pcbuf;
	    norm.filename = "";
	}
    }
    else
    {
	norm.filename = xstr(ev->filename);
	norm.lineno = ev->lineno;
	norm.function = xstr(ev->function);
    }

    return &norm;
}

static void
serialise_uint(int fd, unsigned int i)
{
    write(fd, &i, sizeof(i));
}

static void
serialise_string(int fd, const char *s)
{
    unsigned int len = (s ? strlen(s) : 0);
    serialise_uint(fd, len);
    if (len)
    {
	write(fd, s, len);
	write(fd, "\0", 1);
    }
}

static void
serialise_event(int fd, const u4c_event_t *ev)
{
    serialise_uint(fd, ev->which);
    serialise_string(fd, ev->description);
    serialise_string(fd, ev->filename);
    serialise_uint(fd, ev->lineno);
    serialise_string(fd, ev->function);
}

static int
deserialise_bytes(int fd, char *p, unsigned int len)
{
    int r;

    while (len)
    {
	r = read(fd, p, len);
	if (r < 0)
	    return -errno;
	if (r == 0)
	    return EOF;
	len -= r;
	p += r;
    }
    return 0;
}

static int
deserialise_uint(int fd, unsigned int *ip)
{
    return deserialise_bytes(fd, (char *)ip, sizeof(*ip));
}

static int
deserialise_string(int fd, char *buf, unsigned int maxlen)
{
    unsigned int len;
    int r;
    if ((r = deserialise_uint(fd, &len)))
	return r;
    if (len >= maxlen)
	return -ENAMETOOLONG;
    return deserialise_bytes(fd, buf, len+1);
}

static bool
deserialise_event(int fd, u4c_event_t *ev)
{
    int r;
    unsigned int which;
    unsigned int lineno;
    static char description_buf[1024];
    static char filename_buf[1024];
    static char function_buf[1024];

    if ((r = deserialise_uint(fd, &which)))
	return r;
    if ((r = deserialise_string(fd, description_buf,
				sizeof(description_buf))))
	return r;
    if ((r = deserialise_string(fd, filename_buf,
				sizeof(filename_buf))))
	return r;
    if ((r = deserialise_uint(fd, &lineno)))
	return r;
    if ((r = deserialise_string(fd, function_buf,
				sizeof(function_buf))))
	return r;
    ev->which = which;
    ev->description = description_buf;
    ev->lineno = lineno;
    ev->filename = filename_buf;
    ev->function = function_buf;
    return 0;
}

void __u4c_raise_event(const u4c_event_t *ev, enum u4c_functype ft)
{
    ev = normalise_event(ev);

    if (state->event_pipe)
    {
	/* in child process */
	serialise_event(state->event_pipe, ev);
	serialise_uint(state->event_pipe, ft);
	return;
    }

    /* TODO: notify listener here */
    {
	const char *type;
	char buf[2048];

	switch (ev->which)
	{
	case EV_ASSERT: type = "assert"; break;
	case EV_SYSLOG: type = "syslog"; break;
	case EV_EXIT: type = "exit"; break;
	default: type = "unknown"; break;
	}
	snprintf(buf, sizeof(buf), "u4c: %s %s",
		    type, ev->description);
	if (*ev->filename && ev->lineno)
	    snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
		     " at %s:%u",
		     ev->filename, ev->lineno);
	if (*ev->function)
	    snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
		     " in %s %s",
		     (ft == FT_TEST ? "test" : "fixture"),
		     ev->function);
	strcat(buf, "\n");
	fputs(buf, stderr);
    }
}

static u4c_child_t *
isolate_begin(void)
{
    pid_t pid;
#define PIPE_READ 0
#define PIPE_WRITE 1
    int pipefd[2];
    u4c_child_t *child;
    int delay_ms = 10;
    int max_sleeps = 20;
    int r;

    r = pipe(pipefd);
    if (r < 0)
    {
	perror("u4c: pipe");
	exit(1);
    }

    for (;;)
    {
	pid = fork();
	if (pid < 0)
	{
	    if (errno == EAGAIN && max_sleeps-- > 0)
	    {
		/* rats, we fork-bombed, try again after a delay */
		fprintf(stderr, "u4c: fork bomb! sleeping %u ms.\n",
			delay_ms);
		poll(0, 0, delay_ms);
		delay_ms += (delay_ms>>1);	/* exponential backoff */
		continue;
	    }
	    perror("u4c: fork");
	    exit(1);
	}
	break;
    }

    if (!pid)
    {
	/* child process: return, will run the test */
	close(pipefd[PIPE_READ]);
	state->event_pipe = pipefd[PIPE_WRITE];
	return 0;
    }

    /* parent process */

    fprintf(stderr, "u4c: spawned child process %d\n",
	    (int)pid);
    child = xmalloc(sizeof(*child));
    child->pid = pid;
    close(pipefd[PIPE_WRITE]);
    child->event_pipe = pipefd[PIPE_READ];
    child->next = state->children;
    state->children = child;
    state->nchildren++;

    return child;
#undef PIPE_READ
#undef PIPE_WRITE
}

static bool
isolate_end(u4c_child_t *special, bool fail)
{
    pid_t pid;
    u4c_child_t **prevp, *child;
    int status;
    struct pollfd pfd;
    unsigned int ft;
    int r;
    u4c_event_t ev;

    if (!special)
    {
	fprintf(stderr, "u4c: child process %d finishing\n",
		(int)getpid());
	exit(fail);
    }

    /* Note: too lazy to handle more than 1 child here */
    for (;;)
    {
	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = special->event_pipe;
	pfd.events = POLLIN;
	r = poll(&pfd, 1, -1);
	if (r < 0)
	{
	    perror("u4c: poll");
	    return false;
	}
	/* TODO: implement test timeout handling here */
	if (deserialise_event(special->event_pipe, &ev))
	    break;
	if (deserialise_uint(special->event_pipe, &ft))
	    break;
	__u4c_raise_event(&ev, ft);
    }

    for (;;)
    {
	pid = waitpid(-1, &status, 0);
	if (pid < 0)
	{
	    if (errno == ESRCH || errno == ECHILD)
		fprintf(stderr, "u4c: child %d already reaped!?\n",
			special->pid);
	    else
		perror("u4c: waitpid");
	    return false;
	}
	if (WIFSTOPPED(status))
	{
	    fprintf(stderr, "u4c: process %d stopped on signal %d, ignoring\n",
		    (int)pid, WSTOPSIG(status));
	    continue;
	}
	for (child = state->children, prevp = &state->children ;
	     child && child->pid != pid ;
	     prevp = &child->next, child = child->next)
	    ;
	if (!child)
	{
	    /* some other process */
	    fprintf(stderr, "u4c: reaped stray process %d\n", (int)pid);
	    continue;	    /* whatever */
	}

	fail = false;
	if (WIFEXITED(status))
	{
	    if (WEXITSTATUS(status))
	    {
		fprintf(stderr, "FAIL child process %d exited with %d\n",
			(int)pid, WEXITSTATUS(status));
		fail = true;
	    }
	}
	else if (WIFSIGNALED(status))
	{
	    fprintf(stderr, "FAIL child process %d died on signal %d\n",
		    (int)pid, WTERMSIG(status));
	    fail = true;
	}

	/* detach and clean up */
	*prevp = child->next;
	state->nchildren--;
	close(child->event_pipe);
	free(child);

	/* Are you the one that I've been waiting for? */
	if (child == special)
	    return fail;
    }
}

static void
run_function(u4c_function_t *f)
{
    if (f->type == FT_TEST)
    {
	void (*call)(void) = f->addr;

	call();
    }
    else
    {
	int (*call)(void) = (int(*)(void))f->addr;
	int r;

	r = call();

	if (r)
	{
	    static char cond[64];
	    snprintf(cond, sizeof(cond), "fixture retured %d", r);
	    u4c_throw(event(EV_FIXTURE, cond, f->filename,
			    0, f->name));
	}
    }
}

static void
run_fixtures(u4c_testnode_t *tn,
	     enum u4c_functype type)
{
    u4c_testnode_t *a;
    unsigned int i;

    if (!state->fixtures)
	state->fixtures = xmalloc(sizeof(u4c_function_t*) *
				  state->maxdepth);
    else
	memset(state->fixtures, 0, sizeof(u4c_function_t*) *
				   state->maxdepth);

    /* Run FT_BEFORE from outermost in, and FT_AFTER
     * from innermost out */
    if (type == FT_BEFORE)
    {
	i = state->maxdepth;
	for (a = tn ; a ; a = a->parent)
	    state->fixtures[--i] = a->funcs[type];
    }
    else
    {
	i = 0;
	for (a = tn ; a ; a = a->parent)
	    state->fixtures[i++] = a->funcs[type];
    }

    for (i = 0 ; i < state->maxdepth ; i++)
	if (state->fixtures[i])
	    run_function(state->fixtures[i]);
}

static bool
valgrind_errors(void)
{
    unsigned long leaked = 0, dubious = 0, reachable = 0, suppressed = 0;
    unsigned long nerrors;
    bool fail = false;

    VALGRIND_DO_LEAK_CHECK;
    VALGRIND_COUNT_LEAKS(leaked, dubious, reachable, suppressed);
    if (leaked)
    {
	fprintf(stderr, "FAIL %lu bytes of memory leaked\n", leaked);
	fail = true;
    }

    nerrors = VALGRIND_COUNT_ERRORS;
    if (nerrors)
    {
	fprintf(stderr, "FAIL %lu errors found by valgrind\n", nerrors);
	fail = true;
    }

    return fail;
}

static bool
run_test_code(u4c_testnode_t *tn,
	      const char *fullname)
{
    bool fail = false;
    const u4c_event_t *ev;

    fprintf(stderr, "u4c: running: \"%s\"\n", fullname);

    u4c_try
    {
	run_fixtures(tn, FT_BEFORE);
    }
    u4c_catch(ev)
    {
	__u4c_raise_event(ev, FT_BEFORE);
	fail = true;
    }

    if (!fail)
    {
	u4c_try
	{
	    run_function(tn->funcs[FT_TEST]);
	}
	u4c_catch(ev)
	{
	    __u4c_raise_event(ev, FT_TEST);
	    fail = true;
	}

	u4c_try
	{
	    run_fixtures(tn, FT_AFTER);
	}
	u4c_catch(ev)
	{
	    __u4c_raise_event(ev, FT_AFTER);
	    fail = true;
	}
    }

    if (valgrind_errors())
	fail = true;

    return fail;
}

static void
run_test(u4c_testnode_t *tn)
{
    char *fullname;
    u4c_child_t *child;
    bool fail = false;

    {
	static int n = 0;
	if (++n > 10)
	    return;
    }

    fullname = __u4c_testnode_fullname(tn);
    child = isolate_begin();
    if (!child)
    {
	/* child process */
	fail = run_test_code(tn, fullname);
    }
    fail = isolate_end(child, fail);

    state->nrun++;
    if (fail)
	state->nfailed++;
    else
	fprintf(stderr, "PASS %s\n", fullname);
    xfree(fullname);
}

void
__u4c_run_tests(u4c_testnode_t *tn)
{
    u4c_testnode_t *child;

    if (tn->funcs[FT_TEST])
    {
	run_test(tn);
    }
    else
    {
	for (child = tn->children ; child ; child = child->next)
	    __u4c_run_tests(child);
    }
}

void
__u4c_summarise_results(void)
{
    fprintf(stderr, "u4c: %u run %u failed\n",
	    state->nrun, state->nfailed);
}


