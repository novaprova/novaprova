
Failure Modes
=============

This chapter lists the various failure modes that NovaProva
automatically detects in Code Under Test, in addition to the explicit
assert macro calls that you write in test functions, and describes how
they are reported.

Isolate, Detect, Report
-----------------------

The purpose of a test framework is to discover bugs in code at the
earliest possible time, with the least amount of work by developers.
To achieve this, NovaProva takes an "Isolate, Detect, Report" approach.

- *Isolate*: failure modes are isolated to the test that caused
  them, and thus do not affect other tests.  This helps mitigate
  the problem of cascading spurious test failures, which it harder
  for you to track down which test failures are directly due to
  bugs in the Code Under Test.

- *Detect*: failures are detected using the best automated debugging
  techniques possible.  This reduces the time it takes you to find
  subtle bugs.

- *Report*: failures are reported with as much information as possible,
  in the normal test report.  Ideally, many bugs can be diagnosed by
  examining the test report without re-running the test in a debugger.
  This reduces the time it takes you to diagnose test failures.

Process Per Test
++++++++++++++++

NovaProva uses a strong model of test isolation.  Each test is run in a
separate process, with a central process co-ordinating the starting of
tests and the gathering of results.  This design eliminates a number of
subtle failure modes where running one test can influence another, such
as heap corruption, file descriptor leakage, and global variable
leakage.  The process-per-test model also has the advantage that tests
can be run in parallel, and it allows for test timeouts to be handled
reliably.

Valgrind
++++++++

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

Stack Traces
++++++++++++

NovaProva reports as much information as possible about each failure.
In many cases this includes a stack trace showing the precise point
at which the failure was detected.  When the failure is detected by
Valgrind, often even more information is provided.  For example,
when Valgrind detects that the Code Under Test written some bytes
past the end of a struct allocated with ``malloc()``, it will tell
also give you the stack trace showing where that struct was allocated.


Call to exit()
--------------

NovaProva assumes that the Code Under Test is library code, and that
therefore any call to ``exit()`` is inappropriate.  Thus, any call to the
libc ``exit()`` while running a test will cause the test to fail and
print the exit code and a stack trace.  Note that calls to the
underlying ``_exit()`` system call are *not* detected.

Here's some example test output.

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

NovaProva assumes by default that messages emitted using the libc
``syslog()`` facility are error reports.  Thus any call to ``syslog()``
while a test is running will cause the test to fail immediately,
and the message and a stack trace will be printed.

Here's some example output.

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

Sometimes the Code Under Test is actually expected to emit messages to
``syslog()``.  In these cases you can tell NovaProva to ignore the
message and keep executing the test, using the ``np_syslog_ignore()``
call.  This function takes a UNIX extended regular expression as an
argument; any message which is emitted to ``syslog()`` from that point
onwards in the test will still be reported but will not cause the test
to fail.  You can make multiple calls to ``np_syslog_ignore()``, they
accumulate until the end of the test.  There's no need to remove these
regular expressions, they're automatically removed at the end of the
test.

Here's an example.

.. highlight:: c

::

    static void test_expected(void)
    {
        /* tell NP that a syslog might happen */
        np_syslog_ignore("entirely expected");
        syslog(LOG_ERR, "This message was entirely expected");
    }

When run, this test produces the following output.  Note that
the test still passes.

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

You can achieve more subtle effects than just ignoring messages with
``np_syslog_ignore()`` by using it in combination with
``np_syslog_fail()``.  The latter function also takes a regular
expression which is matched against messages emitted to ``syslog()``,
but it restores the default behavior where a match causes the test to
fail.  Sometimes this can be easier to do than trying to construct
complicated regular expressions.

Finally, if the test
TODO: ``np_syslog_match()`` and ``np_syslog_count()``.


You can of course call any of ``np_syslog_ignore()``,
``np_syslog_fail()`` and ``np_syslog_match()`` in a
setup function (see :ref:`fixtures_chapter` ).


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
