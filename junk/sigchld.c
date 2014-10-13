#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <signal.h>

#define timeval_to_usec(tv) ((tv).tv_sec * 1000000 + (tv).tv_usec)
static int64_t reltime_usec(void)
{
    static int64_t epoch;
    struct timeval now;

    gettimeofday(&now, NULL);
    if (!epoch)
	epoch = timeval_to_usec(now);
    return timeval_to_usec(now) - epoch;
}

static volatile int caught_sigchld = 0;
static void handle_sigchld(int sig __attribute__((unused)))
{
    fprintf(stderr, "parent: [%lld] SIGCHLD delivered\n", reltime_usec());
    caught_sigchld = 1;
}

static void dump_sigmasks(void)
{
#if 1
    fprintf(stderr, "=======> about to sigprocmask()\n");
    sigset_t nset;
    sigfillset(&nset);
    int r = sigprocmask(SIG_SETMASK, &nset, NULL);
    sigemptyset(&nset);
    r = sigprocmask(SIG_SETMASK, &nset, NULL);
    fprintf(stderr, "=======> done with sigprocmask()\n");
#endif
}

int main(int argc, char **argv)
{
    pid_t pid;
    int caught = 0;

#if 0
    fprintf(stderr, "parent: about to call signal()\n");
    void (*oldhandler)(int) = signal(SIGCHLD, handle_sigchld);
    fprintf(stderr, "parent: signal() returned %p\n", oldhandler);
#endif
#if 1
    {
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_sigchld;
	fprintf(stderr, "parent: about to call sigaction()\n");
	int r = sigaction(SIGCHLD, &sa, NULL);
	fprintf(stderr, "parent: sigaction() returned %d\n", r);
    }
#endif

    pid = fork();
    if (pid < 0)
    {
	perror("fork");
	exit(1);
    }
    if (pid)
    {
	/* parent */
	fprintf(stderr, "parent: [%lld] sleeping\n", reltime_usec());
	poll(NULL, 0, 6 * 1000);
	fprintf(stderr, "parent: [%lld] awoke\n", reltime_usec());
	caught |= caught_sigchld ? 1 : 0;
	fprintf(stderr, "parent: caught_sigchld=%d\n", caught_sigchld);
	dump_sigmasks();
	caught |= caught_sigchld ? 2 : 0;
	fprintf(stderr, "parent: caught_sigchld=%d\n", caught_sigchld);
	fprintf(stderr, "\n\n%s\n\n", caught == 3 ? "PASS" : "FAIL");
    }
    else
    {
	/* child */
	fprintf(stderr, "child: [%lld] sleeping\n", reltime_usec());
	usleep(3 * 1000000);
	fprintf(stderr, "child: [%lld] exiting\n", reltime_usec());
	exit(0);
    }

    return 0;
}

