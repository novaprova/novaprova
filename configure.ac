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

AC_INIT(novaprova, 1.2, [gnb@fmeh.org])
AC_CANONICAL_TARGET
# AM_MAINTAINER_MODE
AC_PROG_INSTALL
AC_PROG_RANLIB

libxml=
for p in libxml++-2.6 libxml++ ; do
    PKG_CHECK_MODULES(libxml, $p, [libxml=$p;break])
done
if test x"$libxml" = x ; then
    AC_MSG_ERROR([Cannot find any libxml])
fi
AC_SUBST(libxml)

platform_CFLAGS=

AC_MSG_CHECKING([Platform O/S])
case "$target_os" in
linux-gnu)
    os=linux
    platform_CFLAGS="$platform_CFLAGS -D_GNU_SOURCE"
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
i[34567]86)
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

AC_MSG_CHECKING([Platform specific CFLAGS])
AC_MSG_RESULT($platform_CFLAGS)
AC_SUBST(platform_CFLAGS)

AC_MSG_CHECKING([Platform specific source])
platform_SOURCE="$os.cxx $os"_"$arch.cxx"
AC_MSG_RESULT($platform_SOURCE)
AC_SUBST(platform_SOURCE)

AC_CONFIG_HEADERS([np/util/config.h])
AC_CONFIG_FILES([
    Makefile
    Doxyfile
    novaprova.pc
])
AC_OUTPUT
