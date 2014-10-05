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

AC_INIT(novaprova, 1.3, [gnb@fmeh.org])
AC_CANONICAL_TARGET
# AM_MAINTAINER_MODE
AC_PROG_INSTALL
AC_PROG_RANLIB

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
libbfd_LIBS="-lbfd"
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
    libbfd_CFLAGS="-I/usr/local/include -I/usr/local/include/libiberty"
    libbfd_LIBS="-L/usr/local/lib -lbfd -liberty -lintl -liconv -lz"
    platform_SOURCE="mprotect.cxx darwin_setcontext.S"
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

AC_MSG_CHECKING([Platform specific CFLAGS])
AC_MSG_RESULT($platform_CFLAGS)
AC_SUBST(platform_CFLAGS)

AC_MSG_CHECKING([Platform specific LIBS])
AC_MSG_RESULT($platform_LIBS)
AC_SUBST(platform_LIBS)

AC_MSG_CHECKING([Platform specific source])
platform_SOURCE="$os.cxx $os"_"$arch.cxx $platform_SOURCE"
AC_MSG_RESULT($platform_SOURCE)
AC_SUBST(platform_SOURCE)

AC_CHECK_HEADERS([\
    malloc.h \
    signal.h \
])

valgrind=yes
AC_ARG_ENABLE(valgrind,
	      [Enable the Valgrind runtime memory checker (recommended)],
	      [ valgrind=$enableval ])
if test "$valgrind" = yes ; then
    AC_DEFINE(HAVE_VALGRIND, 1,[whether the runtime memory checker Valgrind is enabled])
fi

debug=no
AC_ARG_ENABLE(debug,
	      [Enable debugging output at compile time],
	      [ debug=$enableval ])
if test "$debug" = yes ; then
    AC_DEFINE(_NP_DEBUG, 1, [Enable debugging output])
fi


dnl TODO: add a --with-bfd option?
AC_SUBST(libbfd_CFLAGS)
AC_SUBST(libbfd_LIBS)

AC_SUBST(sed_extended_opt)

AC_CONFIG_HEADERS([np/util/config.h])
AC_CONFIG_FILES([
    Makefile
    tests/Makefile
    Doxyfile
    novaprova.pc
    plat.sh
])
AC_OUTPUT
