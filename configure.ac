#
#  Copyright 2011-2012 Gregory Banks
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

AC_INIT(novaprova, 1.4, [gnb@fmeh.org])
AC_CANONICAL_TARGET
# AM_MAINTAINER_MODE
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_CXX
AC_PROG_CC

libxml=
for p in libxml-2.0 ; do
    PKG_CHECK_MODULES(libxml, $p, [libxml=$p;break])
done
if test x"$libxml" = x ; then
    AC_MSG_ERROR([Cannot find any libxml])
fi
AC_SUBST(libxml)

platform_SOURCE=
platform_CFLAGS=
platform_LIBS=
libbfd_CFLAGS=
libbfd_LIBS=
sed_extended_opt=

AC_MSG_CHECKING([Platform O/S])
case "$target_os" in
linux-gnu)
    os=linux
    platform_CFLAGS="$platform_CFLAGS -D_GNU_SOURCE"
    platform_LIBS="-ldl -lrt"
    platform_SOURCE="mprotect.cxx"
    sed_extended_opt="-r"
    ;;
darwin*)
    os=darwin
    # we need _XOPEN_SOURCE for setcontext() et al
    platform_CFLAGS="$platform_CFLAGS -D_XOPEN_SOURCE"
    #
    libbfd_CFLAGS=
    libbfd_LIBS=
    # libbfd comes from the homebrew "binutils" keg-only formula
    libbfd_basedir="/usr/local/opt/binutils"
    libbfd_CFLAGS="${libbfd_CFLAGS} -I${libbfd_basedir}/include"
    libbfd_LIBS="${libbfd_LIBS} -L${libbfd_basedir}/lib -lbfd"
    # libiberty comes from a custom keg-only formula in the homebrew-novaprova tap
    libiberty_basedir="/usr/local/opt/libiberty"
    libbfd_CFLAGS="${libbfd_CFLAGS} -I${libiberty_basedir}/include/libiberty"
    libbfd_LIBS="${libbfd_LIBS} -L${libiberty_basedir}/lib -liberty"
    # libintl comes from the homebrew "getext" keg-only formula
    gettext_basedir="/usr/local/opt/gettext"
    libbfd_CFLAGS="${libbfd_CFLAGS} -I${gettext_basedir}/include"
    libbfd_LIBS="${libbfd_LIBS} -L${gettext_basedir}/lib -lintl"
    # libiconv and libz are provided by MacOS
    libbfd_LIBS="${libbfd_LIBS} -liconv -lz"
    #
    platform_SOURCE="mach_vm_protect.cxx darwin_setcontext.S"
    sed_extended_opt="-E"
    ;;
*) AC_MSG_ERROR([Unsupported operating system $target_os]) ;;
esac
AC_DEFINE_UNQUOTED(_NP_OS, "$os", [Name of the Operating System])
dnl defining a macro whose name is not known until configure
dnl runs, doesn't work with AC_DEFINE or AC_DEFINE_UNQUOTED
platform_CFLAGS="$platform_CFLAGS -D_NP_$os"
AC_MSG_RESULT($os)

AC_MSG_CHECKING([Platform architecture])
case "$target_cpu" in
i386|i486|i586|i686)
    arch=x86
    addrsize=4
    maxaddr=0xffffffffUL
    ;;
x86_64)
    arch=x86_64
    addrsize=8
    maxaddr=0xffffffffffffffffULL
    ;;
*) AC_MSG_ERROR([Unsupported architecture $target_cpu]) ;;
esac
AC_DEFINE_UNQUOTED(_NP_ARCH, "$arch", [Name of the Architecture])
platform_CFLAGS="$platform_CFLAGS -D_NP_$arch"
AC_DEFINE_UNQUOTED(_NP_ADDRSIZE, $addrsize, [Size in bytes of an address])
AC_DEFINE_UNQUOTED(_NP_MAXADDR, $maxaddr, [Largest possible address])
AC_MSG_RESULT($arch)

trace=no
AC_ARG_ENABLE(trace,
	      [Enable runtime execution traces],
	      [ trace=$enableval ])
if test "$trace" = yes ; then
    platform_CFLAGS="$platform_CFLAGS -D_NP_ENABLE_TRACE"
fi

case "$arch" in
x86|x86_64)
    AC_MSG_CHECKING([Support for Intel CET])
    saved_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS -fcf-protection=full"
    itworks=no
    AC_TRY_LINK([], [
#if defined(__CET__) && __CET__ >= 3
#else
#error CET support not enabled
#endif
], [ itworks=yes ])
    CFLAGS="$saved_CFLAGS"
    if test "$itworks" = yes ; then
        AC_MSG_RESULT([enabled; disabling])
        # it exists, which is unfortunate because
        # we cannot handle it yet, so disable it
        platform_CFLAGS="$platform_CFLAGS -D_NP_INTEL_CET -fcf-protection=none"
    else
        # phew
        AC_MSG_RESULT(disabled)
    fi
    ;;
esac

AC_MSG_CHECKING([Platform specific CFLAGS])
AC_MSG_RESULT($platform_CFLAGS)
AC_SUBST(platform_CFLAGS)

AC_MSG_CHECKING([Platform specific LIBS])
AC_MSG_RESULT($platform_LIBS)
AC_SUBST(platform_LIBS)

AC_MSG_CHECKING([Platform specific source])
platform_SOURCE="$os.cxx $os"_"$arch.cxx $platform_SOURCE"
case "$arch" in
x86|x86_64) platform_SOURCE="x86x.cxx $platform_SOURCE" ;;
esac
case "$os-$arch" in
linux-x86|linux-x86_64) platform_SOURCE="mprotect_${os}_${arch}.S $platform_SOURCE" ;;
darwin-x86_64) platform_SOURCE="vm_protect_${os}_${arch}.S $platform_SOURCE" ;;
esac
AC_MSG_RESULT($platform_SOURCE)
AC_SUBST(platform_SOURCE)


if test "$libbfd_LIBS" = "" ; then
    dnl Prime the AC cache of compiler behavior
    AC_TRY_LINK([], [return 0])

    AC_MSG_CHECKING([how to link against the BFD library])
    dnl
    dnl For non-Linux platforms we worked out the way to compile
    dnl and link the BFD library above.  Here we handle the
    dnl horrible morass which Linux distros make of this task,
    dnl by trying each of several possible values of $libbfd_LIBS
    dnl until one works.
    dnl
    dnl Several factors contribute to this mess.
    dnl
    dnl The distro can choose to ship libbfd as a static library, a
    dnl dynamic library, or both.  If we choose to use a dynamic
    dnl library, it will have all its downstream dependent libraries
    dnl listed in it, and all we have to do is to link using "-lbfd"
    dnl and we're done.  Thus "-lbfd" is the first choice.
    dnl
    dnl Some distros provide a file which pretends to be a dynamic
    dnl library but is actually a GNU linker script which points at
    dnl the static library and the correct downstream libraries.
    dnl For link line purposes this works just like a dynamic library,
    dnl and all we need is "-lbfd".
    dnl
    dnl In general, there's a horrible downside to using the dynamic
    dnl library.  On Ubuntu, the library is built such that the
    dnl SONAME recorded in our application has the package version
    dnl number of the currently installed binutils package encoded
    dnl in it.  This is, of course, madness.  It means that our
    dnl application will happily link and pass all tests, then some
    dnl weeks or months after installation when binutils is upgraded
    dnl our application will mysteriously fail to start.  The ggcov
    dnl application suffered from this issue.  The fix is to detect
    dnl that we're building on Ubuntu and use the static BFD library,
    dnl i.e. -Wl,-Bstatic -lbfd -liberty -lz -Wl,-Bdynamic.  However
    dnl NovaProva is a special case: the test executables linked against
    dnl BFD are never installed and only have to run immediately after
    dnl being built.  Furthermore they will probably be re-linked almost
    dnl every time they run.  Thus it is unlikely that we will encounter
    dnl binutils being upgraded, and conversely we benefit distinctly
    dnl from the smaller executables and faster link time of dynamic
    dnl libraries.  So here we will always attempt to use the dynamic
    dnl BFD library.
    dnl
    dnl Now, Ubuntu didn't just choose their link scheme to make life
    dnl difficult for us: there is an actual problem they were trying
    dnl to solve.  The problem is that the BFD library does not have a
    dnl stable ABI; the ABI changes at will between binutils versions.
    dnl Other Linux vendors solved this problem differently, by not
    dnl shipping a dynamic BFD library at all, only the static library.
    dnl Because static libraries do not encode their list of downstream
    dnl dependent libraries, we need to add those explicitly to the
    dnl link line.  All the choices after the first choice do this.
    dnl
    dnl The BFD library uses a library of GNU utilities called -liberty,
    dnl which is used by basically nobody else.  Some distros build this
    dnl into libbfd itself, so sometimes we don't need it and sometimes
    dnl we do.
    dnl
    dnl For older versions of the BFD library, all we need is is
    dnl "-lbfd -liberty" and we're done.  However newer versions added
    dnl support for compressing/uncompressing the contents of some
    dnl object file sections.  These newer versions require zlib, so
    dnl we need to try "-lbfd -liberty -lz"
    dnl
    dnl Newer versions of the BFD library also have a plugin feature.
    dnl Leaving aside the insanity, this feature means the BFD library
    dnl depends on the -ldl library.
    dnl

    libbfd_LIBS_1="-lbfd"
    libbfd_LIBS_2="-lbfd -liberty"
    libbfd_LIBS_3="-lbfd -ldl"
    libbfd_LIBS_4="-lbfd -lz"
    libbfd_LIBS_5="-lbfd -ldl -lz"
    libbfd_LIBS_6="-lbfd -liberty -ldl"
    libbfd_LIBS_7="-lbfd -liberty -lz"
    libbfd_LIBS_8="-lbfd -liberty -ldl -lz"

    saved_LIBS="$LIBS"
    i=1
    itworks=no
    while true ; do
	LIBS=`eval echo \\$libbfd_LIBS_$i`
	test -z "$LIBS" && break
	i=`expr $i + 1`
	dnl Yes I know that AC_LINK_IFELSE is a thing but it
	dnl causes aclocal to generate warnings.  Yuck.
	AC_TRY_LINK([#include <bfd.h>], [ bfd_init(); bfd_openr("foo.o", "bar") ], [ itworks=yes ])
	test "$itworks" = yes && break
    done
    if test "$itworks" = yes ; then
	libbfd_LIBS="$LIBS"
	AC_MSG_RESULT($libbfd_LIBS)
    else
	echo ========================================
	cat config.log
	echo ========================================
	AC_MSG_ERROR([Couldnt find a working library link combination for BFD])
    fi
    LIBS="$saved_LIBS"
fi
dnl TODO: add a --with-bfd option?
AC_SUBST(libbfd_CFLAGS)
AC_SUBST(libbfd_LIBS)

dnl Commit http://sourceware.org/git/gitweb.cgi?p=binutils-gdb.git;a=commit;f=bfd/bfd-in2.h;h=52d45da3f275b5d1c8ef2e96a7760585c736133b
dnl which appears in binutils 2.27, incompatibly changed the type of bfd_set_error_handler
dnl in a way which is super hard to test for.  Fortunately that commit also
dnl removed function bfd_get_error_handler, which we don't care about but
dnl removal of a function is a change we can easily test for.
save_LIBS="$LIBS"
save_CFLAGS="$CFLAGS"
LIBS="$LIBS $libbfd_LIBS"
CFLAGS="$CFLAGS $libbfd_CFLAGS"
AC_MSG_CHECKING([type of error handler function])
AC_TRY_LINK_FUNC(bfd_get_error_handler,
[
    AC_MSG_RESULT(printf-like)
],[
    AC_MSG_RESULT(vprintf-like)
    AC_DEFINE(HAVE_BFD_ERROR_HANDLER_VPRINTFLIKE,1,[Whether the BFD error handler callback is vprintf like or printf like])
])
LIBS="$save_LIBS"
CFLAGS="$save_CFLAGS"

debug=yes
AC_ARG_ENABLE(debug,
	      [Enable debugging output at compile time],
	      [ debug=$enableval ])
if test "$debug" = yes ; then
    AC_DEFINE(_NP_DEBUG, 1, [Enable debugging output])
fi

BUILD_DOCS=yes
AC_ARG_ENABLE(docs,
	      [Enable building the documentation at compile time],
	      [ docs=$enableval ])
if test "$docs" = no ; then
  BUILD_DOCS=no
fi
AC_MSG_CHECKING([Build documentation])
AC_MSG_RESULT($BUILD_DOCS)
AC_SUBST(BUILD_DOCS)

AC_ARG_WITH(valgrind,
	    [Enable the Valgrind runtime memory checker (recommended)],
	    [],
	    [with_valgrind=yes])
case "$with_valgrind" in
yes)
    AC_PATH_TOOL([VALGRIND_BINARY], [valgrind])
    ;;
no)
    ;;
*)
    if test -f "$withval" -a -x "$withval" ; then
	VALGRIND_BINARY="$withval"
	with_valgrind=yes
    else
	AC_MSG_ERROR([No Valgrind binary $withval found])
    fi
    ;;
esac

if test "$with_valgrind" = yes ; then
    AC_DEFINE(HAVE_VALGRIND, 1,[whether the runtime memory checker Valgrind is enabled])
    AC_SUBST(HAVE_VALGRIND, 1)
    AC_DEFINE_UNQUOTED(VALGRIND_BINARY, "$VALGRIND_BINARY", [the Valgrind binary])
    PKG_CHECK_MODULES(valgrind, valgrind)
fi

if test "$with_valgrind" = yes ; then
AC_MSG_CHECKING([for valgrind --show-leak-kinds option])
res=`"$VALGRIND_BINARY" --help | grep -o -e '--show-leak-kinds' | head -1`
if test x"$res" != x ; then
    AC_DEFINE(HAVE_VALGRIND_SHOW_LEAK_KINDS, 1,[whether Valgrind has the --show-leak-kinds option])
    AC_MSG_RESULT(yes)
else
    AC_MSG_RESULT(no)
fi
fi

AC_ARG_WITH(valgrind-suppression,
	      [Path to a suppression file to use when running Valgrind],
	      [with_valgrind_suppression=$withval],
	      [with_valgrind_suppression=no])
case "$with_valgrind_suppression" in
yes)
    AC_MSG_ERROR([A file path to the Valgrind suppression file is mandatory])
    ;;
no)
    ;;
*)
	SUPPRESSION_FILE="$withval"
	with_valgrind_suppression=yes
    ;;
esac

if test "$with_valgrind_suppression" = yes ; then
    AC_DEFINE_UNQUOTED(_NP_VALGRIND_SUPPRESSION_FILE, "$SUPPRESSION_FILE", [The Valgrind suppression file path])
fi

AC_CHECK_HEADERS([\
    malloc.h \
    signal.h \
])
AC_SUBST(sed_extended_opt)

AC_CONFIG_HEADERS([np/util/config.h])
AC_CONFIG_FILES([
    Makefile
    tests/Makefile
    Doxyfile
    novaprova.pc
    doc/manual/Makefile
    doc/manual/conf.py
    plat.sh
    uninstalled/bin/pkg-config
    uninstalled/lib/pkgconfig/novaprova.pc
])
AC_OUTPUT

chmod +x uninstalled/bin/pkg-config
