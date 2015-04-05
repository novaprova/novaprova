
.. _porting:

Porting Novaprova
=================

This chapter is intended as a resource for developers wanting to undertake
a port of Novaprova to another platform.

Level of Difficulty
-------------------

The Novaprova library contains significant levels of platform dependency, so
porting it is a non-trivial task.  To undertake a port you will need to have a
working knowledge of details of the following components of the target platform.

- the assembly language
- the C ABI (i.e. function calling conventions and the like)
- some details of the C runtime library (e.g. where process arguments are stashed)
- some details of the kernel ABI (e.g. the shape of the stack frame used to deliver
  signals to user processes).

Many of these details are not documented; you will have to discover them by
reading the system source, or for closed source systems applying reverse
engineering techniques.  Some of these details do not form part of the system
ABI and may be subject to unexpected change.

As a rough guide, porting Novaprova to a new platform is more complex than
porting any C library (except perhaps media codecs) and less complex than
porting Valgrind or the Linux kernel.

Executable File Format
----------------------

Novaprova uses the BFD library from `the GNU binutils
package <http://www.gnu.org/software/binutils/>`_ to handle details of the
executable file format (e.g. ELF on modern Linux systems).  Novaprova uses only
the abstract (i.e.  format-independant) part of the BFD API, and only for a
stricly limited set of tasks (such as discovering segment boundaries and
types).  Hopefully this will require little porting to other executable file
formats (e.g. COFF or Mach objects).

Debugging Information
---------------------

Novaprova depends deeply on the DWARF debugging format.  There is a
considerable body of code which parses DWARF and depends on DWARF formats and
semantics.  If your platform does not use DWARF natively, or cannot be
convinced to by the use of compiler flags such as ``-gdwarf2``, then porting
Novaprova will be very much harder and you should contact the mailing list for
advice.

Valgrind
--------

`Valgrind <http://www.valgrind.org/>`_ is an advanced memory debugging tool
(actually, it's a program simulator which happens debug memory as a side
effect).  Novaprova is designed to make use of Valgrind's powerful bug
discovery features.  If your platform doesn't support Valgrind you're going to
get much less value out of Novaprova than if you had Valgrind, and Novaprova is
going to be missing a great many test failure cases that you really want to be
caught.

For this reason, Novaprova will fail to build without Valgrind.

.. This isnt true on the Darwin branch.  Use this text
.. You'll need to make some minor tweaks to the package specifications to
.. turn off the explicit build time dependency on Valgrind.  The configure.ac
.. script should detect and handle the lack of Valgrind and turn off the
.. features in the code which depend on the presence of Valgrind.


Platform Specific Code
----------------------

Novaprova is written to isolate the platform dependent code to a small set of
files with a well-defined interface.  Partly this is good program practice to
set the scene for future ports (we shall see how well this succeeded when the
first new port is done).  But partly it is due to current necessity, as
Novaprova is sufficiently sensitive to platform details that 32 bit and 64 bit
x86 Linux platforms need different code.

Code Layout
~~~~~~~~~~~

The platform specific code is contained in the C++ namespace
``np::spiegel::platform`` and is implemented in ``.cxx`` files in the directory
``np/spiegel/platform/``.  This follows the usual Novaprova convention where
namespaces and directories have exactly the same shape.

Generally there will be two platform specific files, one containing code which
depends on the OS alone (e.g. ``linux.cxx``) and the other containing code which
depends on the combination of the OS the machine architecture (e.g.
``linux_x86.cxx``).

Build Infrastructure
~~~~~~~~~~~~~~~~~~~~

The ``configure.ac`` script decides which platform specific source files are built.
It can also add compiler flags, so you can use ``#ifdef`` if you really feel the need.

Your first step is to add detection of your platform to ``configure.ac``.  Find
the code that begins

.. highlight:: bash

::

	case "$target_os" in

and add a new case for your platform operating system.

At this point you need to set the ``$os`` variable to the short name of the
platform operating system, e.g. ``x86``.  This is going to be used to choose a
filename ``$os.cxx``, so the name must be short and contain no spaces or / characters.
Ideally it will be entirely lowercase, to match the Novaprova conventions.

Optionally, you can append to the ``$platform_CFLAGS`` variable if there are some
compiler flags (e.g. ``-DGNU_SOURCE``) that should be set for that platform only.
These flags are applied to every C++ file not just the platform specific one.

Your next step is to find the code that begins

.. highlight:: bash

::

	case "$target_cpu" in

and add a new case for your platform hardware architecture.

At this point you need to set three variables.

- ``$arch`` is the short name of the platform hardware architecture,
  e.g. ``x86``.  This is going to be used to construct a filename
  ``${os}_${arch}.cxx`` and to add a compile flag ``-D_NP_$arch`` so
  it must contain only alphanumerics and the underscore character.
  Ideally it will be entirely lowercase, to match the Novaprova conventions.
- ``$addrsize`` is a decimal literal indicating the size of a platform virtual
  address in bytes, e.g. ``4`` for 32-bit platforms.
- ``$maxaddr`` is a C unsigned integer literal indicating the maximum value
  of a virtual address, e.g. ``0xffffffffUL`` on 32-bit platforms.

Optionally, you can also append to the ``$platform_CFLAGS`` variable here.

Finally you should ensure that the following two C++ source files exist.

- ``np/spiegel/platform/${os}.cxx``
- ``np/spiegel/platform/${os}_${arch}.cxx``

.. Darwin branch changes
.. platform specific filenames are now listed in configure.ac not generated
.. which allows for some sharing of code between platforms
.. now have possibly platform specific defines for libbfd, libxml

Platform Specific Functions
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Your next step is to add your implementations of the platform specific
functions to one of those two platform specific files.  Generally you should
add a function to the most general of the two files in which it can be
implemented without using ``#ifdef``.  For example, the function ``get_argv()``
works identically on all Linux platforms so it's implemented in ``linux.cxx``,
whereas ``install_intercept()`` varies widely between 32-bit x86 and 64-bit x86
so it's implemented twice in ``linux_x86.cxx`` and ``linux_x86_64.cxx``.

The remainder of this section will describe the various platform specific
functions, their purpose and the requirements placed upon them.

Get Commandline
+++++++++++++++

.. highlight:: c

::

    bool get_argv(int *argcp, char ***argvp)

Returns in ``*argcp`` and ``*argvp`` the original commandline argument vector for
the process, and ``true`` on success.  Modern C runtime libraries will store the
commandline argument vector values passed to ``main()`` in global variables in
the C library before calling ``main()``. This method retrieves those values so
that Novaprova can use them when forking itself to run Valgrind.  Because no
standard or convention describes these variables, their names are platform
specific; it is also possible on some platforms that no such variables might
exist and the argument vector might need to be deduced by looking in the kernel
aux vector or a filesystem like /proc.

Get Executable Name
+++++++++++++++++++


.. highlight:: c

::

    char *self_exe()

Returns a newly allocated string representing the absolute pathname of the
process' executable.  This is used when Novaprova forks itself to run
Valgrind. The Linux code uses a ``readlink()`` call on the symlink ``/proc/self/exe``.

List Loaded Libraries
+++++++++++++++++++++

.. highlight:: c

::

    vector<linkobj_t> get_linkobjs()

Returns an STL vector of ``linkobj_t`` structures which collectively describe all
the objects dynamically linked into the current executable.  Typically this
means the first ``linkobj_t`` describes the program itself and this is followed
by one ``linkobj_t`` for each dynamically linked library.  This information can
be extracted with a platform specific call into the runtime linker.  For Linux
glibc systems that call is ``dl_iterate_phdr()``.

Normalise an Address
++++++++++++++++++++

.. highlight:: c

::

    np::spiegel::addr_t normalise_address(np::spiegel::addr_t addr)

Takes a virtual address and returns a possibly different virtual address which
is normalized.  Normalized addresses can be used for comparison, i.e. if two
normalized addresses are the same they refer to the same C function.  This
apparently obvious property is not true of function addresses in a dynamically
linked object where the function whose address is being taken is linked from
another dynamic object; the address used actually points into the Procedure
Linkage Table in the calling object.

In order to implement this, the platform specific code needs to know where
the various PLTs are linked into the address space.  The platform specific
function

.. highlight:: c

::

    void add_plt(const np::spiegel::mapping_t &m)

is called from the object handling code to indicate the boundaries of the
PLT in each object.

Remap Text
++++++++++

.. highlight:: c

::

    int text_map_writable(addr_t addr, size_t len)

    int text_restore(addr_t addr, size_t len)

These functions are used when inserting intercepts to ensure that some code in
a ``.text`` or similar segment is mapped writable (modern OSes will map all code
read-only by default for security reasons).  The Linux implementation uses the
``mprotect()`` system call and reference counts pages to allow for the case where
multiple intercepts are installed in the same page.  This code should also work
on most platforms that support the ``mprotect()`` call.

Get A Stacktrace
++++++++++++++++

.. highlight:: c

::

    vector<np::spiegel::addr_t> get_stacktrace()

Returns a stacktrace as a vector of code addresses (``%eip`` samples in x86) of
the calling address, in order from the innermost to the outermost.  The current
(somewhat disappointing) implementation walks stack frames using the frame
pointer, which is somewhat fragile on x86 platforms (where libraries are often
shipped built with the ``-fomit-frame-pointer`` flag, which breaks this
technique).  This function is used only to generate error reports that are read
by humans, so it really should be implemented in a way which emphasizes accuracy
over speed, e.g. using the DWARF2 unwind information to pick apart stack frames
accurately.

Detect Debuggers
++++++++++++++++

.. highlight:: c

::

    bool is_running_under_debugger()

Returns ``true`` if and only if the current process is running under a debugger
such as gdb.  This is needed on some architectures to change the way that
intercepts are implemented; different instructions need to be inserted to avoid
interfering with debugger breakpoints.  Also, some features like test timeouts
are disabled when running under a debugger if they would do more harm than good.
The Linux implementation digs around in the ``/proc`` filesystem to discover
whether the current process is running under ``ptrace()`` and if so compares
the commandline of the tracing process against a whitelist.

Describe File Descriptors
+++++++++++++++++++++++++

.. highlight:: c

::

    vector<string> get_file_descriptors()

Returns an STL vector of STL strings in which the *fd*-th entry is a
human-readable English text description of file descriptor *fd*, or an empty
string if file descriptor *fd* is closed.  This function is called before and
after each test is run to discover file descriptor leaks in test code, so the
returned descriptions should be consistent between calls.  File descriptors used
by Valgrind should not be reported.  The Linux implementation uses the
``/proc/self/fd`` directory.

Install Intercept
+++++++++++++++++

.. highlight:: c

::

    int install_intercept(np::spiegel::addr_t addr, intstate_t &state, std::string &err)

    int uninstall_intercept(np::spiegel::addr_t addr, intstate_t &state, std::string &err)

These functions are the most difficult but most rewarding part of porting
Novaprova.  Intercepts are the key technology that drives advanced Novaprova
features like mocks, redirects, and failpoints.  An intercept is basically a
breakpoint inserted into code, similar to what a debugger uses, but instead of
waking another process when triggered an intercept calls code in the same
process.

These two functions are called to respectively install an intercept at a given
address and remove it again.  The caller normalizes the address and takes care
to only install one intercept at a given address, so for example
``install_intercept`` will not be called twice for the same address without a
call to ``uninstall_intercept``.  The ``intstate_t`` type is defined in the
header file ``np/spiegel/platform/common.hxx`` for all ports (using ``#ifdef``)
and contains any state which might be useful for uninstalling the intercept,
e.g.  the original instructions which were replaced at install time.  The
install function can assume that no Novaprova intercept is already installed at
the given address, but it should take care to handle the case where a debugger
like gdb has independently inserted it's own breakpoint.

Unlike debugger breakpoints, intercepts are always inserted at the first byte of
an instruction, at the beginning of the function prologue.  This can be a useful
simplifying assumption; for example on x86 the first instruction in most
functions is ``pushl %ebp`` whose binary form is the byte 0x55.

The install function will presumably be modifying 1 or more bytes in the
instruction stream to contain some kind of breakpoint instruction; it should
call ``text_map_writable()`` before modifying the bytes to ensure the byte range
is mapped writable.  Similarly the uninstall function should call
``text_restore()`` after restoring the original instruction, to potentially map
the bytes read-only again.  Both functions should call the Valgrind macro
``VALGRIND_DISCARD_TRANSLATIONS()`` after modifying the instruction stream;
Valgrind uses a JIT-like mechanism for caching translated native instructions
and it is important that this cache not contain stale translations.

Both functions return ``0`` on success.  On error they set ``err`` to a
human-readable English error string and return ``-1``.

While an intercept is installed, any attempt to execute the code at ``addr``
should not execute the original code but instead cause a special function
called the trampoline to be called (e.g. via a Unix signal handler).  The
trampoline has the following responsibilities.

#. Extract (from registers, the exception frame on the stack, or the calling
   function's stack frame) the arguments to the intercepted function, and
   store them in an instance of a platform-specific class derived from
   ``np::spiegel::call_t``, which implements the ``get_arg()`` and ``set_arg()``
   methods.

#. Call the static method ``intercept_t::dispatch_before()`` with the intercepted
   address (typically the faulting PC in the exception stack frame) and
   a reference to the ``call_t`` object.

#. Handle any of the possible side effects of ``dispatch_before()``

	#.  If ``call_t.skip_`` is ``true``, arrange to immediately return
	    ``call_t.retval_`` to the calling function, without executing
	    the intercepted function and without calling ``dispatch_after()``.

	#.  If the redirect function ``call_t.redirect_`` is non-zero, arrange to
	    call that instead of the intercepted function.

	#.  Arrange for the intercepted (or redirect) function to be called with
	    the arguments in the ``call_t`` object.

#.  Call the intercepted (or redirect) function.

#.  Store the return value of the intercepted (or redirect) function in
    ``call_t.retval_``.

#.  Call the static method ``intercept_t::dispatch_after()`` with the same
    arguments as ``dispatch_before()``.

#.  Arrange to return ``call_t.retval_`` (which may have been changed as a
    side effect of calling ``dispatch_after()``) to the calling function.

Currently Novaprova intercepts are not required to be thread-safe. This means
that the signal handler and trampoline function can use global state if
necessary.

Exception Handling
++++++++++++++++++

.. highlight:: c

::

    char *current_exception_type()

Returns a new string describing the C++ type name of the exception
currently being handled, or ``0`` if no exception is being handled.


.. highlight:: c

::

    void cleanup_current_exception()

Frees any storage associated with the exception currently being handled.
If this function does nothing, uncaught C++ exceptions reported by Novaprova
will also result in a Valgrind memory leak report.


Utility Functions
-----------------

Some of Novaprova's utility functions have platform-specific features which need
to be considered when porting Novaprova.

POSIX Clocks
~~~~~~~~~~~~

The timestamp code in ``np/util/common.cxx`` relies on the POSIX
``clock_gettime()`` function call, with both the ``CLOCK_MONOTONIC`` and
``CLOCK_REALTIME`` clocks being used.  If your platform does not supply
``clock_gettime()`` then you should write a compatible replacement.  If your
platform does not support a monotonic clock, returning the realtime clock is
good enough.

Page Size
~~~~~~~~~

The memory mapping routines in ``np/util/common.cxx`` use call ``sysconf(_SC_PAGESIZE)``
to retrieve the system page size from the kernel.  This may require a platform-specific
replacement.

Static Library Intercepts
-------------------------

Novaprova also contains a number of functions which are designed to intercept
and change behavior of the standard C library, usually to provide more complete
and graceful detection of test failures.  Some of these functions permanently
replace functions in the standard C library with new versions by defining
functions of the same signature and relying on link order.  Some are runtime
intercepts using the Novaprova intercept mechanism.  Many of these functions are
undocumented or platform-specific, and need to be considered when porting
Novaprova.

__assert
~~~~~~~~

.. highlight:: c

::

    void __assert(const char *condition, const char *filename, int lineno)

This function is called to handle the failure case in the standard ``assert()``
macro.  If it's called, the calling code has decided that an unrecoverable
internal error has occurred.  Usually it prints a message and terminates the
process in such a way that the kernel writes a coredump.  Novaprova defines it's
own version of this routine in ``iassert.c``, which fails the running test
gracefully (including a stack trace message).  The function name and signature
are not defined by any standard.  Systems based on GNU libc also define two
related functions ``__assert_fail()`` and ``__assert_perror_fail()``.

syslog
~~~~~~

Novaprova catches messages sent to the system logging facility and allows test
cases to assert whether specific messages have or have not been emitted.  This
is particularly useful for testing server code.  This is done via a runtime
intercept on the libc ``syslog()`` function.  On GNU libc based systems, the
system ``syslog.h`` header sometimes defines ``syslog()`` as an inline function
that calls the library function ``__syslog_chk()``, so that also needs to be
intercepted.  Similar issues may exist on other platforms.

