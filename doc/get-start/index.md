<!--
  Copyright 2011-2012 Gregory Banks

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
-->
# Getting Started #

## Installation ##

### From The Open Build Service ###

Novaprova is available in installable repositories for many recent Linux
distributions at
[the OpenSUSE Build Service](http://download.opensuse.org/repositories/home:/gnb:/novaprova/).

For Debian/Ubuntu systems, copy and paste the following commands.

~~~~.sh
# Choose a distro
distro=xUbuntu_12.04

# This is the base URL for the NovaProva repo
repo=http://download.opensuse.org/repositories/home:/gnb:/novaprova/$distro

# Download and install the repository GPG key
wget -O - $repo/Release.key | sudo apt-key add -

# Add an APT source
sudo bash -c "echo 'deb $repo ./' > /etc/apt/sources.list.d/novaprova.list"
sudo apt-get update

# Install NovaProva
sudo apt-get install novaprova
~~~~

See [here](http://en.opensuse.org/openSUSE:Build_Service_Enduser_Info) for RedHat and SUSE instructions.

### From A Release Tarball ###

First, download a release tarball.

Novaprova is intended to be built in the usual way that any open source
C software is built.  It has a configure script generated from autoconf,
which you run before building.
To build you need to have g++ and gcc installed.  You will also need to
have the _valgrind.h_ header file installed, which is
typically in a package named something like _valgrind-dev_ .

~~~~.sh
# download the release tarball from http://sourceforge.net/projects/novaprova/files/
tar -xvf novaprova-1.1.tar.bz2
cd novaprova-1.1
./configure
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
autoreconf -iv
./configure
make
make install
~~~~

## Building a Test Executable ##

Because you're testing C code, the first step is to build a test runner
executable.  This executable will contain all your tests and the Code Under Test
and will be linked against the NovaProva
library and whatever other libraries your Code Under Test needs.  Typically, this
is done using the _check:_ make target to both build and run the tests.

~~~~.make
# Makefile

check: testrunner
	./testrunner

TEST_SOURCE= mytest.c
TEST_OBJS=  $(TEST_SOURCE:.c=.o)

testrunner:  $(TEST_OBJS) libmycode.a
	$(LINK.c) -o $@ $(TEST_OBJS) libmycode.a $(NOVAPROVA_LIBS)
~~~~

NovaProva uses the GNOME _pkgconfig_ system to make it easy to find the
right set of compile and link flags.

~~~~.make
NOVAPROVA_CFLAGS= $(shell pkg-config --cflags novaprova)
NOVAPROVA_LIBS= $(shell pkg-config --libs novaprova)

CFLAGS= ... -g $(NOVAPROVA_CFLAGS) ...
~~~~

Note that you only need to compile the test code _mytest.c_
with _NOVAPROVA_CFLAGS_.  NovaProva does *not*
use any magical compile options or do any pre-processing of test code.

However, you should make sure that at least the test code is built with
the _-g_ option to include debugging information.  NovaProva uses that
information to discover tests at runtime.

You do not need to provide a _main_ routine.  NovaProva provides a
default _main_ routine which implements a number of useful command-line
options.  You can write your own later, but you probably won't need to.

The last piece of the puzzle is writing some tests.  Each test is a
single C function which takes no parameters and returns _void_.  Unlike
other unit test frameworks, there's no API to call or magical macro to
use to register tests with the library.  Instead you just name the
function _test_something_, and NovaProva will automatically create a
test called _something_ which calls the function.

For example, let's create a test called _simple_ which tests
the function _myatoi_ which has the same signature and semantics
as the well-known _atoi_ function in the standard C library.

~~~~.c
/* mytest.c */
#include <np.h>	    /* NovaProva library */
#include "mycode.h" /* declares the Code Under Test */

static void test_simple(void)
{
    int r;

    r = myatoi("42");
    NP_ASSERT_EQUAL(r, 42);
}
~~~~

The macro _NP_ASSERT_EQUAL_ checks that it's two integer arguments are
equal, and if not fails the test.  Note that if the assert fails, the
test function terminates immediately.  If the test function gets to it's
end and returns naturally, the test is considered to have passed.

If we build run this test we get output something like this.

~~~~.sh
% make check
./testrunner
np: starting valgrind
np: running
np: running: "simple"
PASS simple
np: 1 run 0 failed
~~~~

As expected, the test passed.  Now let's add another test.  The _myatoi_
function is supposed to convert the initial numeric part of the argument
string, i.e. to stop when it sees a non-numeric character.  Let's feed
it a string which will exercise this behaviour and see what happens.

~~~~.c
static void test_initial(void)
{
    int r;

    r = myatoi("4=2");
    NP_ASSERT_EQUAL(r, 4);
}
~~~~

Running the tests we see:

~~~~.sh
% make check
./testrunner
np: starting valgrind
np: running
np: running: "mytest.simple"
PASS mytest.simple
np: running: "mytest.initial"
EVENT ASSERT NP_ASSERT_EQUAL(r=532, 4=4)
at 0x80529F2: np::spiegel::describe_stacktrace (np/spiegel/spiegel.cxx)
by 0x804C0FC: np::event_t::with_stack (np/event.cxx)
by 0x804B2D2: __np_assert_failed (uasserts.c)
by 0x804AC27: test_initial (mytest.c)
by 0x80522D0: np::spiegel::function_t::invoke (np/spiegel/spiegel.cxx)
by 0x804C731: np::runner_t::run_function (np/runner.cxx)
by 0x804D5C4: np::runner_t::run_test_code (np/runner.cxx)
by 0x804D831: np::runner_t::begin_job (np/runner.cxx)
by 0x804E0D4: np::runner_t::run_tests (np/runner.cxx)
by 0x804E22C: np_run_tests (np/runner.cxx)
by 0x804AB12: main (testrunner.c)

FAIL mytest.initial
np: 2 run 1 failed
make: *** [check] Error 1
~~~~

The first thing we see is that the name of the old test has changed from
_simple_ to _mytest.simple_.  NovaProva organises tests into a tree whose
node names are derived from the test source directory, test source filename,
and test function name.  This tree is pruned down to the smallest possible
size at which the root of the tree is unique.  So when we added a second test
in the same source file, the full name of both tests now includes a component
_mytest_ derived from the name of the source file _mytest.c_.

Note also that the new test failed.  Immediately after the "np: running:"
message we see that the _NP_ASSERT_EQUAL_ macro has failed, and printed both
its arguments as well as a stack trace.  We expected the variable _r_ to equal
to 4 but its actual value at runtime was 532; clearly the _myatoi_ function
did not behave correctly.  We found a bug!

