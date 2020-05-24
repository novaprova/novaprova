
Writing Test Functions
======================

Runtime Discovery
-----------------

Test functions are discovered at runtime using
`Reflection <https://en.wikipedia.org/wiki/Reflection_(computer_programming)>`_.
The NovaProva library walks through all the functions linked into the test
executable and matches those which take no arguments, return ``void``, and
have a name matching one of the following patterns:

* ``test_foo``
* ``testFoo``
* ``TestFoo``

Here's an example of a test function.

.. highlight:: c

::

    #include <np.h>

    static void test_simple(void)
    {
        int r = myatoi("42");
        NP_ASSERT_EQUAL(r, 42);
    }

Note that you do not need to write any code to register this test
function with the framework.  If it matches the above criteria, the
function will be found and recorded by NovaProva.  Just write the
function and you're done.


The Test Tree
-------------

Most other test frameworks provide a simple, 2-level mechanism for
organizing tests; *tests* are grouped into *suites*.

By contrast NovaProva organizes tests into an **tree of test nodes**.
All the tests built into a test executable are gathered at runtime
and are fitted into a tree, with a single common root.  The root is
then pruned until the test names are as short as possible.  Each test
function is a leaf node in this tree (usually).

The locations of tests in this tree are derived from the names of the
test function, the basename of the test source file containing the test
function, and the hierarchy of filesystem directories containing that
source file.  These form a natural classifying scheme that you are
already controlling by choosing the names of filenames and functions.
These names are stuck together in order from least to most specific,
like a Unix filename but separated by ASCII '.' characters, and in
general look like this.

.. highlight:: none

::

    dir.subdir.more.subdirs.filename.function

Here's an example showing how test node names fall naturally out of
your test code organization.

.. highlight:: none

::

    % cat tests/startrek/tng/federation/enterprise.c
    static void test_torpedoes(void)
    {
        fprintf(stderr, "Testing photon torpedoes\n");
    }

    % cat tests/startrek/tng/klingons/neghvar.c
    static void test_disruptors(void)
    {
        fprintf(stderr, "Testing disruptors\n");
    }

    % cat tests/starwars/episode4/rebels/xwing.c
    static void test_lasers(void)
    {
        fprintf(stderr, "Testing laser cannon\n");
    }
    
    % ./testrunner --list
    tests.startrek.tng.federation.enterprise.torpedoes
    tests.startrek.tng.klingons.neghvar.disruptors
    tests.starwars.episode4.rebels.xwing.lasers


Pass and Fail
-------------

A test passes in a very simple way: it returns without failing.  A test
can fail in any number of ways, some of them obvious, all of them
indicative of a bug in the Code Under Test (or possibly the test
itself).  See :ref:`assert_macros` and :doc:`failures` for full details.

Here's an example of a test which always passes.

.. highlight:: c

::

    static void test_always_passes(void)
    {
        printf("Hi, I'm passing!\n");
    }

A test can also use the ``NP_PASS`` macro, which terminates the test
immediately without recording a failure.

.. highlight:: c

::

    static void test_also_always_passes(void)
    {
        printf("Hi, I'm passing too!\n");
        NP_PASS;				    /* terminates the test */
        printf("Now I'm celebrating passing!\n");   /* never happens */
    }

Note that this does not necessarily mean the test will get a Pass
result, only that the test itself thinks it has passed.  It is possible
that NovaProva will detect more subtle failures that the test itself
does not see; some of these failures are not even detectable until after
the test terminates.  So, ``NP_PASS`` is really just a complicated
``return`` statement.

.. highlight:: c

::

    static void test_thinks_it_passes(void)
    {
        void *x = malloc(24);
        printf("Hi, I think I'm passing!\n");
        NP_PASS;	/* but it's wrong, it leaked memory */
    }

A test can use the ``NP_FAIL`` macro, which terminates the test and
records a Fail result.  Unlike ``NP_PASS``, if a test says it fails
then NovaProva believes it.

.. highlight:: c

::

    static void test_always_fails(void)
    {
        printf("Hi, I'm failing\n");
        NP_FAIL;				    /* terminates the test */
        printf("Now I'm mourning my failure!\n");   /* never happens */
    }

Note that NovaProva provides a number of declarative :ref:`assert_macros`
which are much more useful than using ``NP_FAIL`` inside a conditional
statement.  Not only are they more concise, but if they cause a test
failure they provide a more useful error message which helps with
diagnosis.  For example, this test code

.. highlight:: c

::

    static void test_dont_do_it_this_way(void)
    {
        if (atoi("42") != 3)
            NP_FAIL;
    }

    static void test_do_it_this_way_instead(void)
    {
        NP_ASSERT_EQUAL(atoi("42"), 3);
    }

Will generate the following error messages

.. highlight:: none

::

    % ./testrunner

    np: running: "mytests.dont_do_it_this_way"
    EVENT EXFAIL NP_FAIL called
    FAIL mytests.dont_do_it_this_way

    np: running: "mytests.do_it_this_way_instead"
    EVENT ASSERT NP_ASSERT_NOT_EQUAL(atoi("42")=42, 3=3)
    FAIL mytests.do_it_this_way_instead


NovaProva also supports a third test result, Not Applicable, which is
neither a Pass nor a Fail.  A test which runs but decides that some
preconditions are not met, can call the ``NP_NOTAPPLICABLE`` macro.
Such tests are not counted as either passes or failures; it's as if they
never existed.

.. _dependencies:

Dependencies
------------

Some unit test frameworks support a concept of test dependencies, i.e.
the framework knows that some tests should not be run until after some
other tests have been run.  NovaProva does not support test
dependencies.

In the opinion of the author, test dependencies are a terrible idea.
They encourage a style of test writing where some tests are used to
generate external state (e.g. rows in a database) which is then used
as input to other tests.  NovaProva is designed around a model where
each test is isolated, repeatable, and stateless.  This means
that each test must trigger the same behaviour in the Code Under Test
and give the same result, regardless of which order tests were run,
or whether they were run in parallel, or whether any other tests
were run at all, or whether the test had been run before.

The philosophy here is that the purpose of tests is to find bugs
and to keep on finding bugs long after it's written.
If a test is run nightly, fails roughly once a month,
but nobody can figure out why, that test is useless.
So a good test is conceptually simple, easy to run, and easy to diagnose
when it fails.  Deliberately sharing state between tests makes it
harder to achieve all these ideals.

If you find yourself writing a test and you want to save some time
by feeding the results of one test into another, please just stop and
think about what you're doing.

If the Code Under Test needs to be in a particular state before the test
can begin, you should consider it to be the job of the test to achieve
that state from an initial null state.  You can use :doc:`fixtures` to
pull out common code which sets up such state so that you don't have to
repeat it in every test.  You can also use coding techniques which allow
to save and restore the state of the Code Under Test (e.g. a database
dump), and check the saved state into version control along with your
test code.

.. vim:set ft=rst:
