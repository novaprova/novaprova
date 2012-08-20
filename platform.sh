#!/bin/sh
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

uname_m=`uname -m`
uname_o=`uname -o`
extradefines=

case $uname_o in
Linux|GNU/Linux)
    os=linux
    extradefines="-D_GNU_SOURCE"
    ;;
*)
    echo "$0: unknown operating system $uname_o" 1>&2
    exit 1
    ;;
esac

case $uname_m in
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
*)
    echo "$0: unknown architecture $uname_m" 1>&2
    exit 1
    ;;
esac

case "$1" in
--os)
    echo $os
    ;;
--addrsize)
    echo $wordsize
    ;;
--maxaddr)
    echo $maxaddr
    ;;
--arch)
    echo $arch
    ;;
--cflags)
    echo \
	-D_NP_OS=\"\\\"$os\\\"\" \
	-D_NP_ARCH=\"\\\"$arch\\\"\" \
	-D_NP_ADDRSIZE=$addrsize \
	-D_NP_MAXADDR=$maxaddr \
	-D_NP_$os=1 \
	-D_NP_$arch=1 \
	$extradefines
    ;;
--source)
    echo $os.cxx $os"_"$arch.cxx
    ;;
esac

