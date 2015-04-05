
.. _parameters:

Parameters
==========

Parameterized tests are the solution for when you want to run the same test
code several times with just a little bit different each time.

In Novaprova, test parameters are a ``static const char*`` variable attached to
a test node, which is set to one of a number of fixed values by the Novaprova
library.  Parameters are declared with the ``NP_PARAMETER()`` macro.

* and all its possible values
* attached to any node in the test tree
* discovered at runtime using reflection
* tests under that node are run once for each parameter value combination

.. highlight:: c

::

    NP_PARAMETER(pastry, "donut,bearclaw,danish");
    static void test_munchies(void)
    {
        fprintf(stderr, "I'd love a %s\n", pastry);
    }

when run

.. highlight:: none

::

    % ./testrunner

    np: running: "mytest.munchies[pastry=donut]"
    I'd love a donut
    PASS mytest.munchies[pastry=donut]

    np: running: "mytest.munchies[pastry=bearclaw]"
    I'd love a bearclaw
    PASS mytest.munchies[pastry=bearclaw]

    np: running: "mytest.munchies[pastry=danish]"
    I'd love a danish
    PASS mytest.munchies[pastry=danish]

    np: 3 run 0 failed

When multiple parameters apply, all the combinations are iterated

.. highlight:: c

::

    NP_PARAMETER(pastry, "donut,bearclaw,danish");
    NP_PARAMETER(beverage, "tea,coffee");
    static void test_munchies(void)
    {
        fprintf(stderr, "I'd love a %s with my %s\n", pastry, beverage);
    }

when run

.. highlight:: none

::

    % ./testrunner

    np: running: "mytest.munchies[pastry=donut][beverage=tea]"
    I'd love a donut with my tea
    PASS mytest.munchies[pastry=donut][beverage=tea]

    np: running: "mytest.munchies[pastry=bearclaw][beverage=tea]"
    I'd love a bearclaw with my tea
    PASS mytest.munchies[pastry=bearclaw][beverage=tea]

    np: running: "mytest.munchies[pastry=danish][beverage=tea]"
    I'd love a danish with my tea
    PASS mytest.munchies[pastry=danish][beverage=tea]

    np: running: "mytest.munchies[pastry=donut][beverage=coffee]"
    I'd love a donut with my coffee
    PASS mytest.munchies[pastry=donut][beverage=coffee]

    np: running: "mytest.munchies[pastry=bearclaw][beverage=coffee]"
    I'd love a bearclaw with my coffee
    PASS mytest.munchies[pastry=bearclaw][beverage=coffee]

    np: running: "mytest.munchies[pastry=danish][beverage=coffee]"
    I'd love a danish with my coffee
    PASS mytest.munchies[pastry=danish][beverage=coffee]

    np: 6 run 0 failed

.. vim:set ft=rst:
