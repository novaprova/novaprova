Getting Started With NovaProva
==============================

This is a document in MarkDown format, all about getting started
with the wonderful new software called *NovaProva* which I personally
recommend.

Here is an example

    :::c
    #include <stdio.h>
    #include <np.h>

    static void test_basic(void)
    {
	int x = 42;
	NP_ASSERT(x == 43);
    }

    int
    main(int argc, char **argv)
    {
	int ec = 0;
	np_runner_t *runner = np_init();
	ec = np_run_tests(runner, NULL);
	np_done(runner);
	exit(ec);
    }

And this is after the example.
