
Platform Support
================

NovaProva is available on the following platforms.

* Linux x86
* Linux x86_64
* Coming soon: Darwin x86_64

NovaProva supports the GNU compiler up to gcc 4.8, including support for
the DWARF-4 debugging standard.

You may note that this is not a lot of platforms.  The reason is that
the design of NovaProva requires it to deeply dependent on unobvious and
undocumented aspects of the operating system it runs on, particularly
the kernel, the dynamic linker, and the C runtime library.  As such it
is unlikely that it could be successfully ported to an OS for which
source was not available.  Also, NovaProva achieves its greatest value
when paired with Valgrind, which itself is even more dependent on the
underlying OS.  For more information on porting NovaProva to other
platforms, see :ref:`porting`.

.. vim:set ft=rst:
