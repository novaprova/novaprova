# Getting Started #

## Installation ##

### From A Release Tarball ###

First, download a release tarball.

Novaprova is intended to be built in the usual way that any open source
C software is built.  However at the moment it doesn't even use
autotools, so there's no configure script.
To build you need to have g++ and gcc installed.  You will also need to
have the _valgrind.h_ header file installed, which is
typically in a package named something like _valgrind-dev_ .

~~~~.sh
# download the release tarball from http://sourceforge.net/projects/novaprova/files/
tar -xvf novaprova-0.1.tar.bz2
cd novaprova-0.1
make
make install
~~~~

### From Read-Only Git ###

For advanced users only.  Novaprova needs several more tools to build
from a Git checkout than from a release tarball, mainly for the
documentation.  You will need to have
[Python Markdown](http://freewisdom.org/projects/python-markdown/),
[Pygments](http://pygments.org/), and
[Doxygen](http://www.doxygen.org/) installed.  On an Ubuntu system these
are in packages _python-markdown_, _python-pygments_, and _doxygen_
respectively.

~~~~.sh
git clone git://github.com/gnb/novaprova.git
cd novaprova
make
make install
~~~~

## Building a Test Executable ##

Because you're testing C code, the first step is to build a test runner
executable.  This executable will contain all your tests, the Code Under Test
and a small _main_ routine, and will be linked against the NovaProva
library and whatever other libraries your Code Under Test needs.  Typically, this
is done using the _check:_ make target to both build and run the tests.

~~~~.make
# Makefile

check: testrunner
	./testrunner

TEST_SOURCE= mytest.c
TEST_OBJS=  $(TEST_SOURCE:.c=.o)

testrunner:  testrunner.o $(TEST_OBJS) libmycode.a
	$(LINK) -o $@ testrunner.o $(TEST_OBJS) libmycode.a $(NOVAPROVA_LIBS)
~~~~

NovaProva uses the GNOME _pkgconfig_ system to make it easy to find the
right set of compile and link flags.

~~~~.make
NOVAPROVA_CFLAGS= $(shell pkg-config --cflags novaprova-0.1)
NOVAPROVA_LIBS= $(shell pkg-config --libs novaprova-0.1)

CFLAGS= ... -g $(NOVAPROVA_CFLAGS) ...
~~~~

Note that you only need to compile the test code _mytest.c_ and the test
runner code _testrunner.c_ with _NOVAPROVA_CFLAGS_.  NovaProva does *not*
use any magical compile options or do any pre-processing of test code.

However, you should make sure that at least the test code is built with
the _-g_ option to include debugging information.  NovaProva uses that
information to discover tests at runtime.

Your _main_ routine in _testrunner.c_ will initialise the NovaProva
library by calling _np_init_ and then call _np_run_tests_ to do all the
heavy lifting.  Here's a minimal _main_.

~~~~.c
/* testrunner.c */
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
~~~~

The last piece of the puzzle is writing some tests.  Each test is a
single C function which takes no parameters and returns _void_.  Unlike
other unit test frameworks, there's no API to call or magical macro to
use to register tests with the library.  Instead you just name the
function _test_something_, and NovaProva will automatically create a
test called _something_ which calls the function.

For example, let's create a test called _simple_ which does a very
simple test of the function _atoi_ in the standard C library.

~~~~.c
/* mytest.c */
#include <stdlib.h> /* for atoi */
#include <np.h>

static void test_simple(void)
{
    int r;

    r = atoi("42");
    NP_ASSERT_EQUAL(r, 42);
}
~~~~

The macro _NP_ASSERT_EQUAL_ checks that it's two integer arguments are
equal, and if not fails the test.  Note that if the assert fails, the
test function terminates immediately.  If the test function gets to it's
end and returns naturally, the test is considered to have passed.

If we build run this test we get the following output from the
testrunner.

~~~~.sh
~~~~
