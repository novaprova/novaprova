
Platform Support
================

NovaProva is available on the following platforms.

* Linux x86
* Linux x86_64
* Darwin x86_64 (from releaae 1.5)

NovaProva supports the GNU compiler up to gcc 9, including support for
the DWARF-4 debugging standard.  Various versions of the clang compiler
have also been used successfully on some platforms.

From release 1.5, NovaProva needs to be built with a compiler
with C++11 support, for example gcc 4.8.2 or later.  However you don't
need a C++11 compiler, or even a C++ compiler, to *use* NovaProva.

This table describes how NovaProva supports various modern toolchain features.
Some modern platforms enable these toolchain features by default, so to
use NovaProva you may need to disable those features using compiler options.

.. list-table:: Toolchain Feature Support Matrix
   :header-rows: 1

   * - Feature
     - Support
   * - The DWARF-5 standard
     - Unsupported.  Please disable using the ``-gdwarf-4`` compiler option.
   * - Intel CET (Control-flow Enforcement Technology)
     - Unsupported.  Please disable using the ``-fcf-protection=none`` compiler option.
   * - Compressed Debug Info sections
     - Unsupported.  Please disable using the ``-gz=none`` compiler option.
   * - Separate Debug Info files
     - Supported on MacOS (``.dSYM``) since release 1.5.
       Not supported on Linux.
   * - ASLR (Address Space Layout Randomization)
     - Supported since release 1.5.

You may note that this is not a lot of platforms.  The reason is that
the design of NovaProva requires it to be deeply dependent on unobvious and
undocumented aspects of the operating system it runs on, particularly
the kernel, the dynamic linker, and the C runtime library.  As such it
is unlikely that it could be successfully ported to an OS for which
source was not available.  Also, NovaProva achieves its greatest value
when paired with Valgrind, which itself is even more dependent on the
underlying OS.  For more information on porting NovaProva to other
platforms, see :ref:`porting`.

.. vim:set ft=rst:
