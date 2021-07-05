#!/bin/bash
#
#  Copyright 2011-2021 Gregory Banks
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

# configure.sh has all the logic for running configure
# before compiling NovaProva

function fatal()
{
    echo "$0: $*" 1>&2
    exit 1
}

#
# Parse arguments
#
compiler=gcc
valgrind=no

function usage()
{
    echo "Usage: $0 [--compiler gcc|clang] [--enable-valgrind]"
    exit 1
}

while [ $# -gt 0 ] ; do
    case "$1" in
    --compiler) compiler="$2" ; shift ;;
    --enable-valgrind) valgrind=yes ;;
    --disable-valgrind) valgrind=no ;;
    --help) usage ;;
    -*) usage ;;
    *) usage ;;
    esac
    shift
done

set -ex
automake -ac || echo 'please ignore the automake error about a missing Makefile.am'
autoreconf -iv
if [ "$compiler" = clang ] ; then
    export CC=clang
    export CXX=clang++
fi
if [ "$valgrind" = no ] ; then
    valgrind_opt="--without-valgrind"
else
    valgrind_opt=
fi
./configure $valgrind_opt

