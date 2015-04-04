

Running Tests Under GDB
-----------------------

Novaprova detects when the test executable is being run under a
debugger such as ``gdb``, and takes some actions to make this
an easier experience for you.

* Test timeouts are disabled, to avoid tests spuriously
  failing while you are debugging them.
* Valgrind is disabled, because ``gdb`` and Valgrind usually
  interact poorly.  Also, Novaprova needs to use different
  techniques to implement mocking depending on whether
  Valgrind or gdb is present.

To use ``gdb`` effectively you should set some ``gdb`` options.

::

    set follow-fork-mode child
    set detach-on-fork off
    handle SIGSEGV nostop noprint

Unused Functions
----------------

recent gcc optimizes out un-called static functions.  Use NP_USED or
make the functions extern

Attaching Functions to Body Test Nodes
--------------------------------------

foo/bar/baz.c
foo/bar.c

this aliasing effect is deliberate



The Runner Object
-----------------

    np_runner_t \*np_init(void);

Initialise the NovaProva library.  Returns a new runner object.

You should call <code>np_init()</code> to initialise NovaProva before running
any tests.  It discovers tests in the current executable, and returns
a pointer to a <code>np_runner_t</code> object which you can pass to
<code>np_run_tests()</code> to actually run the tests.

The first thing the function does is to ensure that the calling
executable is running under Valgrind, which involves re-running
the process.  So be aware that any code between the start of <code>main()</code>
and the call to <code>np_init()</code> will be run twice in two different
processes, the second time under Valgrind.

    void np_done(np_runner_t \*);

Shut down the NovaProva library.  Destroys the given runner object and shuts down the library.

    int np_run_tests(np_runner_t \*, np_plan_t \*);

Uses the <code>runner</code> object to run all the tests described in the <code>plan</code>
object.  If <code>plan</code> is NULL, a temporary default plan is created which
will result in all the discovered tests being run in testnode tree order.

    void np_list_tests(np_runner_t \*, np_plan_t \*);
    void np_set_concurrency(np_runner_t \*, int);
    bool np_set_output_format(np_runner_t \*, const char \*);
    int np_get_timeout(void);   /\* in seconds, or zero \*/

