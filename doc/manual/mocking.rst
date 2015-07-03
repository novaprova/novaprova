
.. _mocking:

Mocking
=======

One of the big problems when writing unit tests is how to pick
apart the morass of interconnections between modules to find
a subset of your program which can be tested by itself.  To
construct an example, let's say you want to test the ``foo``
module, but it makes calls to the ``bar`` module, which in
turn does a REST call to the ``baz`` server.  How do you
unit test ``foo``?

One answer is to test from the edges of the dependency graph
inward, i.e. first test the ``baz`` server, then test the
``bar`` module using a live ``baz`` server, then finally test the
``foo`` module using live ``bar`` and ``baz``.  This can work, but
it has a number of problems:

- the program may have loops in the dependency graph,
  e.g. the ``baz`` server calls into the ``foo`` module.

- it may be difficult or impossible to get a component
  into a reproducible initial state, e.g. ``baz`` is an
  Oracle database.

- you may need to test a module's response to unusual
  behaviors in it's downstream modules, e.g. you have to test
  that ``foo`` handles ``bar`` returning a particular error code.

The better answer is: mocking.

What Is Mocking?
----------------

Mocking is a technique used only in test builds, to replace
modules we don't want to be testing with fake (*mock*) versions
which present the same interfaces but have simpler and more
controllable behavior.  The process of replacing is called
*mocking* and the replaced functions are *mocked*.

Let's construct an example.  Let's say one of the functions
in our ``foo`` module looks like this:

.. highlight:: c

::

    void foo_mustache(int x)
    {
        BarTxn *txn = bar_begin();
        int r = bar_shoreditch(x);
        if (r < 0)
        {
            fprintf(stderr, "shoreditch failed: %d", -r);
            return;
        }
        bar_commit(txn);
    }

There's a common type of bug in this code.  If the ``bar_shoreditch()``
function returns an error, the ``BarTxn`` object created by the earlier
call to ``bar_begin()`` is leaked.  But the bug only appears if
``bar_shoreditch()`` returns with an error, and this hardly ever happens
with the real ``bar`` module.

To write a unit test that tickles this bug, we need to mock the
``bar_shoreditch()`` function.  We want to create a version of that
function which always returns an error, and arrange for it to be
called whenever the Code Under Test tries to call the real
``bar_shoreditch()``.  Here's a way to do that using NovaProva

.. highlight:: c

::

    int mock_bar_shoreditch(int x)
    {
        if (x == 42)
            return -ENOENT;
        return 0;
    }

How It Works
------------

When NovaProva finds a function whose name starts with the letters
``mock_``, it automatically adds that function as a mock to all the
tests defined in the same source file (more precisely, the mock is
attached to the testnode corresponding to the file).  Mocks are
automatically installed before their tests start and are automatically
uninstalled again when their tests finish.  While those tests are
running, any attempt to call ``bar_shoreditch()`` from any part of the
test function or the Code Under Test, will instead call the mocked
function ``mock_bar_shoreditch()`` which then simulates whatever
behavior we want for the test (in this case, return an error if the
input is 42).  Any other tests are unaffected by this behavior; if they
call ``bar_shoreditch()`` that will call the real ``bar_shoreditch()``.

The mechanism that NovaProva uses operates at runtime, not at link time
like some C mocking libraries.  Nor does it rely on C++ virtual
functions, like some other mocking libraries.  Instead it uses a very
platform-specific mechanism similar to a debugger breakpoint, which
causes any call to the original function to trap into some special
NovaProva handler code which calls the mock function.  This mechanism
is incredibly powerful and has a number of implications.

Adding Mocks Dynamically
------------------------

Mocks can be inserted and removed partway through a test.  This
allows mocking only some of the calls to a particular function, and
letting other calls go through to the real implementation.  For
example

.. highlight:: c

::

    static int fail_with_enoent(int x)
    {
        return -ENOENT;
    }

    void test_some_ditches(void)
    {
        foo_mustache(0);    /* calls the real bar_shoreditch() */
        foo_mustache(1);
        np_mock(bar_shoreditch, fail_with_enoent);
        foo_mustache(2);    /* calls the mock, leaks BarTxn */
        np_unmock(bar_shoreditch);
        foo_mustache(3);    /* calls the real bar_shoreditch() again */
    }

Using Many Simple Mocks
-----------------------

Because mocks can be added partway through a test, you can write a test
which uses different mock functions in different parts of the test.
This usually means that each mock function can be simple and easy to
understand, instead of trying to write a single big complicated mock
function with lots of logic designed to handle all the different tests
you will ever use.  For example

.. highlight:: c

::

    static int fail_with_enoent(int x)
    {
        return -ENOENT;
    }
    static int fail_with_econnrefused(int x)
    {
        return -ECONNREFUSED;
    }
    static int fail_with_eaccess(int x)
    {
        return -EACCESS;
    }

    void test_failing_all_over(void)
    {
        np_mock(bar_shoreditch, fail_with_enoent);
        foo_mustache(4);
        np_mock(bar_shoreditch, fail_with_econnrefused);
        foo_mustache(5);
        np_mock(bar_shoreditch, fail_with_eaccess);
        foo_mustache(6);
    }

Note that you don't need to explicitly call ``np_unmock()`` - any mocks
installed dynamically during the test are automatically uninstalled
after the test finishes.

Failure Injection
-----------------

You can use mocks to implement simple forms of failure injection.  For
example, there's a second common type of bug in the ``foo_mustache()``
implementation above: it doesn't check for a NULL return from
``bar_begin()``, which might happen in rare cases like a memory
allocation failure.  Here's how you would test for that bug:

.. highlight:: c

::

    static void *returns_null(size_t sz)
    {
        return NULL;
    }
    void test_malloc_failure(void)
    {
        foo_mustache(7);
        np_mock(malloc, returns_null);
        foo_mustache(8);    /* malloc() call in bar_begin() fails */
        np_unmock(malloc)
        foo_mustache(9);
    }


Mocking By Name
---------------

In the above examples you saw how you can mock a function using
the function's address.  NovaProva will also let you mock a function
using the function's name.  You don't even need to be able to call
function normally, so you can mock static functions in other modules
(as long as that function has a known and unique name).  For example:

.. highlight:: c

::

    /* this function is not visible outside the bar module
     * and is called from bar_begin() */
    static BarTxn *bar_txn_alloc(void)
    {
        BarTxn *txn = (BarTxn *)malloc(sizeof(BarTxn));
        if (!txn)
            return NULL;
        txn->id = nextid++;
        txn->state = FETAL;
        txn->items = NULL;
        return txn;
    }

    /* in test code */
    void test_txn_alloc_failure(void)
    {
        np_mock_by_name("bar_txn_alloc", returns_null);
        foo_mustache(10);       /* bar_txn_alloc() returns NULL */
    }

.. vim:set ft=rst:
