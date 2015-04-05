
C API Reference
===============

Result Macros
-------------

These macros can be used in test functions to indicate a particular
test result.

.. doxygengroup:: result_macros
   :content-only:

.. _assert_macros:

Assert Macros
-------------

These macros can be used in test functions to check a particular
condition, and if the check fails print a helpful message and FAIL
the test.  Treat them as you would the standard ``assert`` macro.

.. doxygengroup:: assert_macros
   :content-only:

Syslog Matching
---------------

These functions can be used in a test function to
control the how the test behaves if the Code Under Test
attempts to emit messages to ``syslog``.  See :ref:`syslog`
for more information.

.. doxygengroup:: syslog
   :content-only:

Parameters
----------

These functions can be used to set up parameterized tests.
See :ref:`parameters` for more information.

.. doxygengroup:: parameters
   :content-only:

Dynamic Mocking
---------------

These functions can be used in a test function to
dynamically add and remove mocks.  See :ref:`mocking`
for more information.

.. doxygengroup:: mocking
   :content-only:

Main Routine
------------

These functions are for writing your own ``main()`` routine.
You probably won't need to use these, see :ref:`main_routine`.

.. doxygengroup:: main
   :content-only:

Miscellany
----------

.. doxygengroup:: misc
   :content-only:

.. vim:set ft=rst:
