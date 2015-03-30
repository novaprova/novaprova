
Mocking
=======

mocking a function = arrange for Code under Test to call a special test
function instead of some normal part of CuT

dynamic: inserted at runtime not link time
uses a breakpoint-like mechanism

mocks may be setup declaratively
name a function mock_foo() & it mocks foo()

mocks may be added at runtime
by name or by function pointer
from tests or fixtures
so you can mock for only part of a test


mocks may be attached to any testnode
automatically installed before any test below that node starts
automatically removed afterward



Static Mocks
------------

Functions named mock_foo()

Dynamic Mocks
-------------

* np_mock()
* np_unmock()
* np_mock_by_name()

.. vim:set ft=rst:
