
.. _parameters:

Parameters
==========

Parameterized tests are the solution for when you want to run the same
test code several times with just a little bit different each time.

In NovaProva, test parameters are a ``static const char*`` variable
attached to a test node, which is set to one of a number of fixed values
by the NovaProva library.  Parameters are declared with the
``NP_PARAMETER()`` macro.  This macro takes two arguments: the name of
the variable and a string containing all the possible values, separated
by commas or whitespace.

The parameter is attached to the level of the test node tree where it's
declared, and applies to all the tests at that level or below, e.g. all
the tests declared in the same ``.c`` file.  Those tests are run once
for each value of the parameter, with the parameter's variable set to a
different value each time.  In test results, the name and value of the
parameter are shown inside ``[]`` appended to the full test name.

For example, this parameter declaration

.. highlight:: c

::

    NP_PARAMETER(pastry, "donut,bearclaw,danish");
    static void test_munchies(void)
    {
        fprintf(stderr, "I'd love a %s\n", pastry);
    }

will result in the ``test_munchies()`` function being called three
times, like this

.. highlight:: none

::

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

When multiple parameters apply, the test functions are called once
for each of the combinations of the parameters.  For example, this
pair of parameters

.. highlight:: c

::

    NP_PARAMETER(pastry, "donut,bearclaw,danish");
    NP_PARAMETER(beverage, "tea,coffee");
    static void test_munchies(void)
    {
        fprintf(stderr, "I'd love a %s with my %s\n", pastry, beverage);
    }

will result in the ``test_munchies()`` function being called six
times, like this

.. highlight:: none

::

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
