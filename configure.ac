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

AC_MSG_CHECKING([Platform is supported])
$CONFIG_SHELL platform.sh --verify || AC_MSG_ERROR([Unsupported platform])

libxml=
for p in libxml++-2.6 libxml++ ; do
    PKG_CHECK_MODULES(libxml, $p, [libxml=$p;break])
done
if test x"$libxml" = x ; then
    AC_MSG_ERROR([Cannot find any libxml])
fi
AC_SUBST(libxml)


AC_MSG_CHECKING([Platform specific CFLAGS])
platform_CFLAGS="`$CONFIG_SHELL platform.sh --cflags`"
AC_MSG_RESULT($platform_CFLAGS)
AC_SUBST(platform_CFLAGS)

AC_MSG_CHECKING([Platform specific source])
platform_SOURCE="`$CONFIG_SHELL platform.sh --source`"
AC_MSG_RESULT($platform_SOURCE)
AC_SUBST(platform_SOURCE)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
    Makefile
    Doxyfile
    novaprova.pc
])
AC_OUTPUT
