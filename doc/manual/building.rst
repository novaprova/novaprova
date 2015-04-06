
Building a Test Executable
==========================

What is a Test Executable?
--------------------------

Because you're testing C code, the first step is to build a test runner
executable.  This executable will contain all your tests and the Code
Under Test and will be linked against the NovaProva library and whatever
other libraries your Code Under Test needs.  When you want to run all
or one of your tests, you run this executable with various arguments.

Setting Up the Makefile
-----------------------

Most C and C++ software is built using the venerable ``make`` utility,
these days usually the GNU make implementation.  While you can use any
names you like, the GNU project defines a standard set of target names
and their semantics, which you would be advised to stick to.  The target
``check:`` is what you should be using to both build and run the tests.

Here is a fragment of an example Makefile.  It assumes your code to
be tested has been built into a local archive library ``libmycode.a``,
but it also works if you replace that with an explicit list of
separate object files.

.. highlight:: make

::

    NOVAPROVA_CFLAGS= $(shell pkg-config --cflags novaprova)
    NOVAPROVA_LIBS= $(shell pkg-config --libs novaprova)
    
    CFLAGS= ... -g $(NOVAPROVA_CFLAGS) ...

    check: testrunner
            ./testrunner
    
    TEST_SOURCE= mytest.c
    TEST_OBJS=  $(TEST_SOURCE:.c=.o)
    
    testrunner:  $(TEST_OBJS) libmycode.a
            $(LINK.c) -o $@ $(TEST_OBJS) libmycode.a $(NOVAPROVA_LIBS)

NovaProva uses the GNOME ``pkgconfig`` system to make it easy to find the
right set of compile and link flags.

Note that you only need to compile the test code ``mytest.c`` with
``NOVAPROVA_CFLAGS``, and link with the NovaProva library.   NovaProva does
*not* use any magical compile options or do any pre-processing of
test code or the Code Under Test.  All the magic happens at runtime.

However, you should make sure that at least the test code is built with
the ``-g`` option to include debugging information.  NovaProva uses that
information to discover tests.

Using GNU Automake
------------------

Many C developers prefer to use GNU automake to build their projects.
One good reason is that it's by far the easiest technique to build shared
libraries in a cross-platform manner.  NovaProva can be used to run
tests in an automake-based project too.

First, ensure that your ``configure.ac`` has the following

::

    dnl configure.ac

    AC_INIT(15_automake, 1.4)
    AM_INIT_AUTOMAKE([serial-tests])
    AC_PROG_CC
    AC_PROG_RANLIB

    PKG_CHECK_MODULES(NOVAPROVA, novaprova)

    AC_CONFIG_HEADERS([config.h])
    AC_CONFIG_FILES([Makefile])
    AC_OUTPUT

.. highlight:: make

Note the use of the ``serial-tests`` automake option.  Recent versions
of automake feature a new "parallel tests" feature, which is enabled by
default.  This feature is complicated to use and provides no benefit at
all when used with NovaProva, as NovaProva implements it's own parallelism
and doesn't need help from automake.  The ``serial-tests`` option disables
the feature.  If you're running with an older version of automake which
does not offer the parallel test feature, you do not need to specify the
option.

Note also the use of the ``PKG_CHECK_MODULES`` autoconf macro.  This
sets up the variables ``NOVAPROVA_CFLAGS`` and ``NOVAPROVA_LIBS``
to the correct value for compiling and linking with the NovaProva
library.

Next, ensure your Makefile.am contains something like the following.

::

    # Makefile.am

    lib_LIBRARIES=	libmycode.a
    libmycode_a_SOURCES=	mycode.c

    # Tell automake to build the testrunner on "make check"
    check_PROGRAMS=	testrunner
    # Tell automake to run the testrunner on "make check"
    TESTS= $(check_PROGRAMS)

    # List all your test source code here
    testrunner_SOURCES= mytest.c
    # Compile only test code with -g and the NovaProva flags
    testrunner_CFLAGS= -g $(NOVAPROVA_CFLAGS)
    # Link testrunner with the Code Under Test and the NovaProva library
    testrunner_LDADD= libmycode.a $(NOVAPROVA_LIBS)


Now when you run ``make check``, make will build the Code Under Test,
build the test code, link the test runner, and run the test runner.

.. _main_routine:

Main Routine
------------

You do not need to provide a ``main()`` routine for the test executable
to link.  The NovaProva library provides a default ``main()`` routine
which implements a number of useful command-line options.  This section
describes the behavior of test executables built with the default
``main()``.

Note, you can always write your own ``main()`` later, but you probably
won't need to.  NovaProva has a hierarchical :doc:`fixtures` feature
which you should probably use instead.

Test Executable Usage
---------------------

Here is a description of the test executable usage.

|    **./testrunner --list**
|    **./testrunner** [**-j** *number*] [**-f** *format*] [*test_spec*...]

**-f** *format*, **--format** *format*
    Set the format in which test results will be emitted.  See
    :doc:`output-formats` for a list of available formats.

**-j** *number*, **--jobs** *number*
    Set the maximum number of test jobs which will be run at the same
    time, to *number*.  The default value is 1, meaning tests will be run
    serially.  A value of 0 is shorthand for one job per online CPU in
    the system, which is likely to be the most efficient use of the
    system.

**-l**, **--list**
    Instead of running any tests, print to stdout the fully qualified
    names of all the test functions (i.e. leaf test nodes) known to
    NovaProva, and exit.

*test_spec*
    The fully qualified name of a test node (i.e. a test, a
    test source file file, or a directory containing test source files).
    All the tests at or below the test node will be run.  Tests are
    started in test node traversal order.  If no tests are specified, all
    the tests known to NovaProva will be run.


.. vim:set ft=rst:
