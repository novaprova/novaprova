
Failure Modes
=============

This chapter lists the various failure modes that NovaProva detects in
Code Under Test, in addition to explicit NP assert macros,
and describes how they are reported.

Isolate and Detect
------------------

The purpose of a test framework is to discover bugs in code at the
earliest possible time, with the least amount of work by developers.
To achieve this, NovaProva takes an "Isolate & Detect" approach,
meaning that failure modes are firstly isolated to the test that caused
them (thus not affecting other tests) and secondly detected using
the best automated debugging techniques possible.

NovaProva uses a strong model of test isolation.  Each test is run in a
separate process, with a central process co-ordinating the starting of
tests and the gathering of results.  This design eliminates a number of
subtle failure modes where running one test can influence another, such
as heap corruption, file descriptor leakage, and environment variable
leakage.  The process-per-test model also has the advantage that tests
can be run in parallel, and it allows for test timeouts to be handled
reliably.

All tests are run using the `Valgrind <http://www.valgrind.org>`_ memory
debugging tool, which enables detection of a great many subtle runtime
errors not otherwise detectable.

The use of Valgrind is on by default and is handled silently by the
NovaProva library.  Normally, running a program under Valgrind requires
the use of a wrapper script or special care in the Makefile, but with
NovaProva all you have to do is to run the test executable.

NovaProva also detects when the test executable is being run under a
debugger such as ``gdb``, and avoids using Valgrind.  This is because
``gdb`` and Valgrind usually interact poorly, and it's best to use only
one or the other.

The use of Valgrind can also be manually disabled by using the
``NOVAPROVA_VALGRIND`` environment variable before running the test
executable.  This is not a recommended practice.

.. highlight: bash

::

    # NOT RECOMMENDED
    export NOVAPROVA_VALGRIND=no
    ./testrunner
    
The downside of all this isolation and debugging is that tests can run
quite slowly.


Call to exit()
--------------

NP library defines an exit()
calling exit() during a test
prints a stack trace fails the test
calling exit() outside a test calls _exit()

.. highlight:: none

::

    np: running: "mytest.exit"
    About to call exit(37)
    EVENT EXIT exit(37)
    at 0x8056522: np::spiegel::describe_stacktrace
    by 0x804BD9C: np::event_t::with_stack
    by 0x804B0CE: exit
    by 0x804AD42: test_exit
    by 0x80561F0: np::spiegel::function_t::invoke
    by 0x804C3A5: np::runner_t::run_function
    by 0x804D28A: np::runner_t::run_test_code
    by 0x804D4F7: np::runner_t::begin_job
    by 0x804DD9A: np::runner_t::run_tests
    by 0x804DEF2: np_run_tests
    by 0x804ACF2: main
    ￼FAIL mytest.exit
    np: 1 run 1 failed


.. _syslog:

Messages emitted to syslog()
----------------------------

dynamic function intercept on syslog()
installed before each test uninstalled after each test
test may register regexps for expected syslogs
unexpected syslogs fail the test

.. highlight:: none

::

    np: running: "mytest.unexpected_syslog"
    EVENT SLMATCH err: This message shouldn't happen
    at 0x8059B22: np::spiegel::describe_stacktrace
    by 0x804DD9C: np::event_t::with_stack
    by 0x804B9FC: np::mock_syslog
    by 0x807A519: np::spiegel::platform::intercept_tramp
    by 0x804B009: test_unexpected_syslog
    by 0x80597F0: np::spiegel::function_t::invoke
    by 0x804FB99: np::runner_t::run_function
    by 0x8050A7E: np::runner_t::run_test_code
    by 0x8050CEB: np::runner_t::begin_job
    by 0x805158E: np::runner_t::run_tests
    by 0x80516E6: np_run_tests
    by 0x804AFC3: main
    FAIL mytest.unexpected_syslog

And to tell NP to ignore a specific syslog

.. highlight:: c

::

    static void test_expected(void)
    {
        /* tell NP that a syslog might happen */
        np_syslog_ignore("entirely expected");
        syslog(LOG_ERR, "This message was entirely expected");
    }

.. highlight:: none

::

    np: running: "mytest.expected"
    EVENT SYSLOG err: This message was entirely expected
    at 0x8059B22: np::spiegel::describe_stacktrace
    by 0x804DD9C: np::event_t::with_stack
    by 0x804BABA: np::mock_syslog
    by 0x807A519: np::spiegel::platform::intercept_tramp
    by 0x804B031: test_expected
    by 0x80597F0: np::spiegel::function_t::invoke
    by 0x804FB99: np::runner_t::run_function
    by 0x8050A7E: np::runner_t::run_test_code
    by 0x8050CEB: np::runner_t::begin_job
    by 0x805158E: np::runner_t::run_tests
    by 0x80516E6: np_run_tests
    by 0x804AFC3: main
    PASS mytest.expected

Failed calls to libc assert()
-----------------------------

Invalid memory accesses
-----------------------

e.g. following a null pointer

Valgrind notices first and emits a useful analysis

* stack trace
* line numbers
* fault address
* info about where the fault address points

child process dies with SIGSEGV or SIGBUS.
test runner process reaps it & fails the test

.. highlight:: none

::

    np: running: "mytest.segv"
    About to follow a NULL pointer Valgrind report
    ==32587== Invalid write of size 1
    ==32587==
    ...
    ==32587==
    ==32587==
    ==32587== Address 0x0 is not stack'd, malloc'd or (recently) free'd
    ==32587== Process terminating with default action of signal 11 (SIGSEGV)
    ==32587== Access not within mapped region at address 0x0
    EVENT SIGNAL child process 32587 died on signal 11
    at 0x804AD40: test_segv (mytest.c:22)
    by 0x804DEF6: np_run_tests (runner.cxx:665)
    by 0x804ACF6: main (testrunner.c:31)
    FAIL mytest.crash_and_burn
    np: 1 run 1 failed


Buffer overruns
---------------

buffer overruns, use of uninitialised variables
discovered by Valgrind, as they happen which emits its usual helpful analysis

Memory leaks
------------

discovered by Valgrind, explicit leak check after each test completes

test process queries Valgrind
	unsuppressed errors → the test has failed

.. highlight:: none

::

    np: running: "mytest.memleak"
    Valgrind report
    About to do leak 32 bytes from malloc()
    Test ends
    ==779== 32 bytes in 1 blocks are definitely lost in loss record 9 of 54
    ==779==    at 0x4026FDE: malloc ...
    ==779==    by 0x804AD46: test_memleak (mytest.c:23)
    ...
    ==779==    by 0x804DEFA: np_run_tests (runner.cxx:665)
    ==779==    by 0x804ACF6: main (testrunner.c:31)
    EVENT VALGRIND 32 bytes of memory leaked
    FAIL mytest.memleak
    np: 1 run 1 failed

File Descriptor leaks
---------------------

Use of uninitialized variables
------------------------------

Looping, deadlocked, or slow tests
----------------------------------

NP detects when its running under gdb and automatically disables the
test timeout and Valgrind.

per-test timeout
test fails if the timeout fires

default timeout 30 sec
3 x when running under Valgrind
disabled when running under gdb

implemented in the testrunner process
child killed with SIGTERM

.. highlight:: none

::

    np: running: "mytest.slow"
    Test runs for 100 seconds
    Have been running for 0 sec
    Have been running for 10 sec
    Have been running for 20 sec
    Have been running for 30 sec
    Have been running for 40 sec
    Have been running for 50 sec
    Have been running for 60 sec
    Have been running for 70 sec
    Have been running for 80 sec
    EVENT TIMEOUT Child process 2294 timed out, killing
    EVENT SIGNAL child process 2294 died on signal 15
    FAIL mytest.slow
    np: 1 run 1 failed

C++ Exceptions
--------------

.. vim:set ft=rst:
