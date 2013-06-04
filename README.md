# Licence #

Copyright 2011-2013 Gregory Banks

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

# What is NovaProva #

*NovaProva* is a unit testing framework for C (and later C++). In other
words it discovers tests, runs them in various ways, detects failures
in them, and reports the failures out to a human.

There are many other such frameworks for many languages, see
http://en.wikipedia.org/wiki/XUnit  Notably, we use the CUnit today
in the Cyrus IMAP server, and it was CUnit's many pain points that
inspired my work on *NovaProva*.

# Why is NovaProva cooler than the rest? #

*NovaProva* is better than existing unit test frameworks for C or C++
in the following ways.

* Other frameworks require code to be written to register a test
  function with the framework.  NP discovers tests at runtime using
  reflection (*NovaProva* reads DWARF debug information from the executable
  it's linked into). This makes it easier to add tests; just write a
  function whose name starts with "test_" and link it into the test
  binary.

* Likewise fixture setup and teardown routines (code that is run before
  and after each test to manage common resources that each test needs).

* Other frameworks organise tests into named groups commonly called
  "suites"; *NovaProva* extends this concept to an arbitrary tree-like
  namespace. This allows finer control of various test features like
  fixtures. The test tree is created automatically from the function
  and file names of discovered test routines.

* *NovaProva* safely detects and reports on a great many modes
  of C code failure which slip by most frameworks, such as memory
  overruns, uninitialised variables, memory leaks, deadlocks and
  loops, segmentation violations, calling exit(), calling syslog().
  Much of this is done by enforcing that the test program runs under
  the Valgrind memory checker; the library detects when it's not running
  under Valgrind and re-runs the executable under Valgrind.

* *NovaProva* allows the test writer do fully dynamic function
  interception (mocking, in xUnit speak) using a runtime technology
  similar to gcc breakpoints.  This works without modifying the code
  to be intercepted, without access to the code to be intercepted.
  It also doesn't use link-time tricks so it works with functions that
  are not visible to the linker, and it means that mocking can be turned
  on or off at any time, including partway through a test.

More documentation is in the source, or you can
[read it online](http://www.novaprova.org/docs.html).


# Getting started - ubuntu guide #

### Required packages for build ###

* build-essential
* autoconf
* automake
* libtool
* libxml++2.6-dev
* valgrind
* binutils-dev
* python-pygments
* python-markdown
* doxygen

### Build instructions ###

    autoreconf
    ./configure
    make
    make docs # optional
    make install

### Checking your install ###

Build and check the getting started example.

    cd doc/get-start/example1/
    make check

The result of the check should look similiar to this (build output supressed).

    ./testrunner
    [0.000] np: starting valgrind
    np: NovaProva Copyright (c) Gregory Banks
    np: Built for O/S linux architecture x86_64
    np: running
    np: running: "mytest.simple"
    PASS mytest.simple
    np: running: "mytest.initial"
    EVENT ASSERT NP_ASSERT_EQUAL(r=532, 4=4)
    at 0x417E0B: np::spiegel::describe_stacktrace
    by 0x4049FE: np::event_t::with_stack
    by 0x4043FE: __np_assert_failed
    by 0x403DF3: test_initial
    by 0x4179BD: np::spiegel::function_t::invoke
    by 0x4092C8: np::runner_t::run_function
    by 0x409DC7: np::runner_t::run_test_code
    by 0x40A061: np::runner_t::begin_job
    by 0x40810E: np::runner_t::run_tests
    by 0x40A27A: np_run_tests
    by 0x404786: main

    FAIL mytest.initial
    np: 2 run 1 failed
    make: *** [check] Error 1

Fix the bug in mycode.c.

    -    for ( ; *s ; s++)
    +    for ( ; *s >= '0' && *s <= '9' ; s++)

Build and check the example again.

    make check

Now both tests should succeed (build output suppressed).

    ./testrunner
    [0.000] np: starting valgrind
    np: NovaProva Copyright (c) Gregory Banks
    np: Built for O/S linux architecture x86_64
    np: running
    np: running: "mytest.simple"
    PASS mytest.simple
    np: running: "mytest.initial"
    PASS mytest.initial
    np: 2 run 0 failed

It works!!!
