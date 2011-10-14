#include "common.h"
#include "except.h"
#include "u4c_priv.h"

typedef struct u4c_text_listener u4c_text_listener_t;
struct u4c_text_listener
{
    u4c_listener_t super;
    unsigned int nrun;
    unsigned int nfailed;
    u4c_result_t result; /* for the current test */
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
text_begin(u4c_listener_t *l)
{
    u4c_text_listener_t *tl = container_of(l, u4c_text_listener_t, super);
    tl->nrun = 0;
    tl->nfailed = 0;
    fprintf(stderr, "u4c: running\n");
}

static void
text_end(u4c_listener_t *l)
{
    u4c_text_listener_t *tl = container_of(l, u4c_text_listener_t, super);
    fprintf(stderr, "u4c: %u run %u failed\n",
	    tl->nrun, tl->nfailed);
}

static void
text_begin_node(u4c_listener_t *l __attribute__((unused)),
		const u4c_testnode_t *tn)
{
    u4c_text_listener_t *tl = container_of(l, u4c_text_listener_t, super);
    char *fullname = __u4c_testnode_fullname(tn);

    fprintf(stderr, "u4c: running: \"%s\"\n", fullname);
    tl->result = R_UNKNOWN;
    xfree(fullname);
}

static void
text_end_node(u4c_listener_t *l __attribute__((unused)),
	      const u4c_testnode_t *tn)
{
    u4c_text_listener_t *tl = container_of(l, u4c_text_listener_t, super);
    char *fullname = __u4c_testnode_fullname(tn);

    tl->nrun++;
    switch (tl->result)
    {
    case R_PASS:
	fprintf(stderr, "PASS %s\n", fullname);
	break;
	break;
    case R_FAIL:
	tl->nfailed++;
	fprintf(stderr, "FAIL %s\n", fullname);
	break;
    default:
	fprintf(stderr, "??? (result %d) %s\n", tl->result, fullname);
	break;
    }
    xfree(fullname);
}

static void
text_add_event(u4c_listener_t *l __attribute__((unused)),
	       const u4c_event_t *ev, enum u4c_functype ft)
{
//     u4c_text_listener_t *tl = container_of(l, u4c_text_listener_t, super);
    const char *type;
    char buf[2048];

    switch (ev->which)
    {
    case EV_ASSERT: type = "ASSERT"; break;
    case EV_EXIT: type = "EXIT"; break;
    case EV_SIGNAL: type = "SIGNAL"; break;
    case EV_SYSLOG: type = "SYSLOG"; break;
    case EV_FIXTURE: type = "FIXTURE"; break;
    case EV_VALGRIND: type = "VALGRIND"; break;
    default: type = "unknown"; break;
    }
    snprintf(buf, sizeof(buf), "EVENT %s %s",
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

static void
text_finished(u4c_listener_t *l, u4c_result_t res)
{
    u4c_text_listener_t *tl = container_of(l, u4c_text_listener_t, super);
    tl->result = res;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static u4c_listener_ops_t text_ops =
{
    .begin = text_begin,
    .end = text_end,
    .begin_node = text_begin_node,
    .end_node = text_end_node,
    .add_event = text_add_event,
    .finished = text_finished
};

u4c_listener_t *
__u4c_text_listener(void)
{
    static u4c_text_listener_t l = {
	    .super = { .next = NULL, .ops = &text_ops }};
    return &l.super;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
