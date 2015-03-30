

Running Tests Under GDB
-----------------------

Novaprova detects when the test executable is being run under a
debugger such as ``gdb``, and takes some actions to make this
an easier experience for you.

* Test timeouts are disabled, to avoid tests spuriously
  failing while you are debugging them.
* Valgrind is disabled, because ``gdb`` and Valgrind usually
  interact poorly.  Also, Novaprova needs to use different
  techniques to implement mocking depending on whether
  Valgrind or gdb is present.

To use ``gdb`` effectively you should set some ``gdb`` options.

::

    set follow-fork-mode child
    set detach-on-fork off
    handle SIGSEGV nostop noprint

Unused Functions
----------------

recent gcc optimizes out un-called static functions.  Use NP_USED or
make the functions extern

Attaching Functions to Body Test Nodes
--------------------------------------

foo/bar/baz.c
foo/bar.c

this aliasing effect is deliberate
