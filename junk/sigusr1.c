#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
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

static volatile int caught_sigusr1 = 0;
static void handle_sigusr1(int sig __attribute__((unused)))
{
    fprintf(stderr, "parent: [%lld] SIGUSR1 delivered\n", reltime_usec());
    caught_sigusr1 = 1;
}

static void dump_sigmasks(void)
{
#if 0
    fprintf(stderr, "=======> about to system()\n");
    system("ls /foo");
    fprintf(stderr, "=======> system() returned\n");
#endif
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
    void (*oldhandler)(int) = signal(SIGUSR1, handle_sigusr1);
    fprintf(stderr, "parent: signal() returned %p\n", oldhandler);
#endif
#if 1
    {
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_sigusr1;
	fprintf(stderr, "parent: about to call sigaction()\n");
	int r = sigaction(SIGUSR1, &sa, NULL);
	fprintf(stderr, "parent: sigaction() returned %d\n", r);
    }
#endif
#if 0
    {
	sigset_t nset;
	sigfillset(&nset);
	r = sigprocmask(SIG_SETMASK, &nset, NULL);
	sigemptyset(&nset);
	r = sigprocmask(SIG_SETMASK, &nset, NULL);
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
#if 0
	fprintf(stderr, "parent: [%lld] waiting\n", reltime_usec()); dump_sigmasks(); usleep(1 * 1000000);
	fprintf(stderr, "parent: [%lld] waiting\n", reltime_usec()); dump_sigmasks(); usleep(1 * 1000000);
	fprintf(stderr, "parent: [%lld] waiting\n", reltime_usec()); dump_sigmasks(); usleep(1 * 1000000);
	fprintf(stderr, "parent: [%lld] waiting\n", reltime_usec()); dump_sigmasks(); usleep(1 * 1000000);
	fprintf(stderr, "parent: [%lld] waiting\n", reltime_usec()); dump_sigmasks(); usleep(1 * 1000000);
	fprintf(stderr, "parent: [%lld] waiting\n", reltime_usec()); dump_sigmasks(); usleep(1 * 1000000);
	fprintf(stderr, "parent: [%lld] usleep returned\n", reltime_usec());
#endif
#if 1
	fprintf(stderr, "parent: [%lld] sleeping\n", reltime_usec());
	usleep(6 * 1000000);
	fprintf(stderr, "parent: [%lld] awoke\n", reltime_usec());
#endif
	caught |= caught_sigusr1 ? 1 : 0;
	fprintf(stderr, "parent: [%lld] caught_sigusr1=%d\n", reltime_usec(), caught_sigusr1);
	dump_sigmasks();
	caught |= caught_sigusr1 ? 2 : 0;
	fprintf(stderr, "parent: [%lld] caught_sigusr1=%d\n", reltime_usec(), caught_sigusr1);
	usleep(6 * 1000000);
	fprintf(stderr, "parent: exiting\n");

	fprintf(stderr, "\n\n%s\n\n", caught == 3 ? "PASS" : "FAIL");
    }
    else
    {
	/* child */
	fprintf(stderr, "child: sending signal to parent\n");
	pid_t ppid = getppid();
	kill(ppid, SIGUSR1);
	usleep(3 * 1000000);
	fprintf(stderr, "child: exiting\n");
	exit(0);
    }

    return 0;
}

