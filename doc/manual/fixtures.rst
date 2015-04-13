
.. _fixtures_chapter:

Fixtures
========

When you write a lot of tests you will sooner or later find yourself
pasting the same code over and over into multiple tests.  Typically this
is code which initializes the Code Under Test and the environment it
runs in, getting it into a state where the tests can usefully be run.
Examples include creating files, setting environment variables, and
populating a database.  Nobody likes repeated code, so we have a
problem.

It's tempting to solve the problem by doing one of two things:

1. Pull the initialization code out of the tests entirely, say into
   a separate shell script, and let the tests just assume that the
   environment is initialized.

2. Write a test which does the initialization, and then make
   the other tests depend on it.  See :ref:`dependencies`.

Both of these are bad ideas because they clash with the principles
that tests should be *isolated* and *self-contained*.

A better approach would be to write a function which does all the setup
(a *setup function*) and call it from each test function.  In general
there will also be some other code to undo the effects of setup function
and cleanup the environment again, this could go into a *teardown
function* which is called at the end of every test.

This is much better, but there are still some issues.  Most pressingly,
if a test fails then the call from the end of the test function to the
teardown function will never happen.  If the setup function creates a
resource that has a lifetime outside of it's process, like a file or a
shared memory segment, that resource will be leaked.

NovaProva, like the xUnit family of test frameworks, provides a feature
called `fixtures <http://en.wikipedia.org/wiki/XUnit#Test_fixtures>`_,
to make this easy and correct.

A fixture is simply a matched pair of setup and teardown functions
attached to a test node, with the following automatic behavior
implemented in the framework:

* The setup function on a test node is automatically called before
  any test function at or below that test node is called.
* The teardown function on a test node is automatically called after
  any test function at or below that test node is called.
* If the setup function fails, the test function and the teardown function
  are not run.
* If the test function is run, the teardown function will always be run,
  regardless of whether the test function succeeded or finished.
* If either the setup or teardown function fails the test is marked FAILed.
* Either the setup or teardown functions may be missing.
* If setup functions are defined for multiple ancestor test nodes of
  a test function, the setup functions are run from the rootmost
  to the leafmost, i.e. from the least specific to the most specific.
* Multiple teardown functions are run in the reverse order, i.e.
  from leafmost to rootmost.

Like test functions, setup and teardown functions are discovered at
runtime using Reflection.  They are functions which take no arguments
and return an integer, with any of the following names.

* ``setup``
* ``Setup``
* ``set_up``
* ``init``
* ``Init``
* ``teardown``
* ``tearDown``
* ``Teardown``
* ``TearDown``
* ``tear_down``
* ``cleanup``
* ``Cleanup``

Here's an example of defining fixture functions.

.. highlight:: c

::

    static int set_up(void)
    {
        fprintf(stderr, "Setting up fixtures\n");
        return 0;
    }

    static int tear_down(void)
    {
        fprintf(stderr, "Tearing down fixtures\n");
        return 0;
    }

Here's an example of fixtures running.

.. highlight:: none

::

    np: running
    np: running: "mytest.one"
    Setting up fixtures
    Running test_one
    Finished test_one
    Tearing down fixtures
    PASS mytest.one
    np: running: "mytest.two"
    Setting up fixtures
    Running test_two
    Finished test_two
    Tearing down fixtures
    PASS mytest.two
    np: 2 run 0 failed



.. vim:set ft=rst:
