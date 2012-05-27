#include <stdlib.h>
#include <unistd.h>
#include <np.h>	    /* NovaProva library */

int
main(int argc, char **argv)
{
    int ec = 0;
    np_runner_t *runner;

    /* Initialise the NovaProva library */
    runner = np_init();

    /* Run all the discovered tests */
    ec = np_run_tests(runner, NULL);

    /* Shut down the NovaProva library */
    np_done(runner);

    exit(ec);
}

