
Fixtures
========

This chapter will discuss fixtures, how they're discovered, how they're
arranged hierarchically, how they're run and in what order, what happens
if they fail, and provides examples of usage.

Setup functions are called before the test

Teardown functions are called after the test

Like test functions, discovered at runtime using Reflection.  They are
functions which take no arguments and return an integer, with any of
the following names.

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
