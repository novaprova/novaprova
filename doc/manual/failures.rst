
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


Call To exit()
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

Messages Emitted To syslog()
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
onwards in the test that matches the regular expression, will be
silently ignored and will not cause the test
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
the test passes and the message does not appear.

.. highlight:: none

::

    np: running: "mytest.expected"
    PASS mytest.expected

This behavior changed in version 1.4 (commit 6234a2a). Before
that, the call to ``syslog()`` would result in an ``EVENT`` and a
stacktrace rather than being silently ignored.

You can achieve more subtle effects than just ignoring messages with
``np_syslog_ignore()`` by using it in combination with
``np_syslog_fail()``.  The latter function also takes a regular
expression which is matched against messages emitted to ``syslog()``,
but it restores the default behavior where a match causes the test to
fail.  Sometimes this can be easier to do than trying to construct
complicated regular expressions.

Finally, if the test depends on the Code Under Test generating
(or not generating) specific messages, you can use ``np_syslog_match()``
which tells NovaProva to just count any matching messages, and
``np_syslog_count()`` to discover that count and assert on its value.
The behavior of ``np_syslog_match()`` changed in version 1.4
(commit ef2f3b4).  Before that, the call to ``syslog()`` would result in
an ``EVENT`` and a stacktrace rather than being silently counted.

You can of course call any of ``np_syslog_ignore()``,
``np_syslog_fail()`` and ``np_syslog_match()`` in a
setup function (see :ref:`fixtures_chapter` ).


Failed Calls To libc assert()
-----------------------------

The standard library's ``assert()`` macro is sometimes used in the Code
Under Test to check for conditions which must be true or the program is
fatally flawed, e.g. preconditions, or the internal consistency of data
structures.  If the condition is false, the macro prints a message and
exits the running process by calling ``abort()``.  NovaProva catches
this occurrence, prints a more useful error message than the default
(including a stack trace), and gracefully fails the test.

Here's an example.

.. highlight:: c

::

    static void test_assert(void)
    {
        int white = 1;
        int black = 0;
        assert(white == black);
    }

When run, this test produces the following output and the test fails.

.. highlight:: none

::

    np: running: "tnassert.assert"
    EVENT ASSERT white == black
    at 0x41827F: np::spiegel::describe_stacktrace (np/spiegel/spiegel.cxx)
    by 0x40555C: np::event_t::with_stack (np/event.cxx)
    by 0x404CCD: __assert_fail (iassert.c)
    by 0x4049F2: test_assert (tnassert.c)
    by 0x417E0B: np::spiegel::function_t::invoke (np/spiegel/spiegel.cxx)
    by 0x409E04: np::runner_t::run_function (np/runner.cxx)
    by 0x40A83D: np::runner_t::run_test_code (np/runner.cxx)
    by 0x40AB06: np::runner_t::begin_job (np/runner.cxx)
    by 0x408DD6: np::runner_t::run_tests (np/runner.cxx)
    by 0x40AD16: np_run_tests (np/runner.cxx)
    by 0x40529A: main (main.c)

    FAIL tnassert.assert


Invalid Memory Accesses
-----------------------

One of the plague spots of coding in C is the ease with which
the Code Under Test can accidentally perform invalid memory accesses
such as following a null pointer.

When this happens under NovaProva, Valgrind detects it first and
emits a useful analysis containing:

* a stack trace,
* line numbers,
* the fault address, and
* information about where the fault address points.

NovaProva then gracefully fails the test.  Here's an example:

.. highlight:: none

::

    np: running: "mytest.segv"
    About to follow a NULL pointer
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


Buffer Overruns
---------------

Buffer overruns are when C code accidentally walks off the end
of a buffer, corrupting memory beyond the buffer.  This is a classic
security vulnerability and an important class of errors in C programs.

When this happens under NovaProva, Valgrind detects it first and
emits a useful analysis.  Depending on the exact failure mode,
Valgrind might either just print the analysis or it might deliver
a SEGV to the program.  In either case, NovaProva catches it and
gracefully fails the test.  Here's an example:

.. highlight:: none

::

    np: running: "tnoverrun.heap_overrun_small"
    about to overrun a buffer by a small amount
    ==6986== Invalid write of size 1
    ==6986==    at 0x4C29D28: memcpy (mc_replace_strmem.c:882)
    ==6986==    by 0x4049E8: do_a_small_overrun (tnoverrun.c:22)
    ==6986==    by 0x404A8E: test_heap_overrun_small (tnoverrun.c:39)
    ==6986==    by 0x4164A5: np::spiegel::function_t::invoke(std::vector<np::spiegel::value_t, std::allocator<np::spiegel::value_t> >) const (spiegel.cxx:606)
    ==6986==    by 0x4085F0: np::runner_t::run_function(np::functype_t, np::spiegel::function_t*) (runner.cxx:526)
    ==6986==    by 0x409029: np::runner_t::run_test_code(np::job_t*) (runner.cxx:650)
    ==6986==    by 0x4092F2: np::runner_t::begin_job(np::job_t*) (runner.cxx:710)
    ==6986==    by 0x4075B6: np::runner_t::run_tests(np::plan_t*) (runner.cxx:147)
    ==6986==    by 0x409502: np_run_tests (runner.cxx:822)
    ==6986==    by 0x404CDE: main (main.c:102)
    ==6986==  Address 0x6b58801 is 1 bytes after a block of size 32 alloc'd
    ==6986==    at 0x4C27A2E: malloc (vg_replace_malloc.c:270)
    ==6986==    by 0x404A5A: test_heap_overrun_small (tnoverrun.c:36)
    ==6986==    by 0x4164A5: np::spiegel::function_t::invoke(std::vector<np::spiegel::value_t, std::allocator<np::spiegel::value_t> >) const (spiegel.cxx:606)
    ==6986==    by 0x4085F0: np::runner_t::run_function(np::functype_t, np::spiegel::function_t*) (runner.cxx:526)
    ==6986==    by 0x409029: np::runner_t::run_test_code(np::job_t*) (runner.cxx:650)
    ==6986==    by 0x4092F2: np::runner_t::begin_job(np::job_t*) (runner.cxx:710)
    ==6986==    by 0x4075B6: np::runner_t::run_tests(np::plan_t*) (runner.cxx:147)
    ==6986==    by 0x409502: np_run_tests (runner.cxx:822)
    ==6986==    by 0x404CDE: main (main.c:102)
    ==6986== 
    overran
    EVENT VALGRIND 2 unsuppressed errors found by valgrind
    FAIL tnoverrun.heap_overrun_small


Use Of Uninitialized Variables
------------------------------

The accidental use of uninitialised variables is yet another of
C's awful failure modes.

When this happens under NovaProva, Valgrind detects it first and emits a useful
analysis.  Then NovaProva catches it and gracefully fails the test.  Here's an
example:

.. highlight:: none

::

    np: running: "tnuninit.uninitialized_int"
    ==6020== Conditional jump or move depends on uninitialised value(s)
    ==6020==    at 0x404A07: test_uninitialized_int (tnuninit.c:27)
    ==6020==    by 0x4175C9: np::spiegel::function_t::invoke(std::vector<np::spiegel::value_t, std::allocator<np::spiegel::value_t> >) const (spiegel.cxx:606)
    ==6020==    by 0x40983C: np::runner_t::run_function(np::functype_t, np::spiegel::function_t*) (runner.cxx:526)
    ==6020==    by 0x40A275: np::runner_t::run_test_code(np::job_t*) (runner.cxx:650)
    ==6020==    by 0x40A53E: np::runner_t::begin_job(np::job_t*) (runner.cxx:710)
    ==6020==    by 0x408802: np::runner_t::run_tests(np::plan_t*) (runner.cxx:147)
    ==6020==    by 0x40A74E: np_run_tests (runner.cxx:822)
    ==6020==    by 0x405172: main (main.c:102)
    ==6020==
    EVENT VALGRIND 1 unsuppressed errors found by valgrind
    FAIL tnuninit.uninitialized_int
    np: 1 run 1 failed


Memory Leaks
------------

The accidental leaking of memory which is allocated but never freed, is yet
another of C's awful failure modes.

NovaProva asks Valgrind to do an explicit memory leak check after each test
finishes; Valgrind will print a report showing how much memory was leaked and
the stack trace of where each leak was allocated.  If the test caused memory
leaks, NovaProva fails the test.  Here's an example:

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

File Descriptor Leaks
---------------------

A more subtle kind of resource leak is a file descriptor leak.  This typically
happens in code which reads a file, encounters an error condition, and while
handling the error forgets to ``fclose()`` the file.  This kind of problem can be
very insidious in long-running server code.

NovaProva detects file descriptor leaks by scanning the test child process'
file descriptor table before and after each test and looking for leaks.  If the
test (or any of the fixture code) caused a file descriptor leak, NovaProva
fails the test.  Here's an example:

.. highlight:: none

::

    np: running: "tnfdleak.leaky_test"
    MSG leaking fd for .leaky_test.dat
    EVENT FDLEAK test leaked file descriptor 5 -> /build/novaprova/tests/.leaky_test.dat
    FAIL tnfdleak.leaky_test
    np: running: "tnfdleak.leaky_fixture"
    MSG leaking fd for .leaky_fixture.dat
    EVENT FDLEAK test leaked file descriptor 3 -> /build/novaprova/tests/.leaky_fixture.dat
    FAIL tnfdleak.leaky_fixture
    np: 2 run 2 failed


Looping, Deadlocked, Or Slow Tests
----------------------------------

Sometimes the Code Under Test enters an infinite loop, or causes a deadlock
between two or more threads.  NovaProva uses a per-test timeout to detect
these cases; if the test runs longer than the timeout NovaProva will kill
the child test process with ``SIGTERM`` and gracefully fail the test.

The basic test timeout is 30 seconds.  NovaProva automatically detects and
adjusts the timeout in certain situations.  When the test executable is being
run under gdb, NovaProva disables the test timeout.  When the test executable
is being run under Valgrind (the default behavior), NovaProva triples the
timeout.

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

A failure mode unique to C++ is the uncaught exception.  NovaProva catches
all uncaught exceptions, by setting up a global default terminate handler.
If the Code Under Test throws an exception which is not caught, NovaProva will
print a message including the exception type, the result of ``e.what()`` if
the exception is of a subclass of ``std::exception``, and the stacktrace of
the ``throw`` statement.  NovaProva will then gracefully fail the test.

.. highlight:: none

::

    np: running: "mytest.slow"
    np: running: "tnexcept.uncaught_exception"
    MSG before call to bar
    EVENT EXCEPTION terminate called with exception foo::exception: Oh that went badly
    at 0x416C2D: np::spiegel::describe_stacktrace (np/spiegel/spiegel.cxx)
    by 0x426FCA: np::event_t::with_stack (np/event.cxx)
    by 0x426A7B: __np_terminate_handler (iexcept.c)
    ...
    FAIL tnexcept.uncaught_exception


.. vim:set ft=rst:
