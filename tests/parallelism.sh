#!/bin/bash
#
#  Copyright 2011-2020 Gregory Banks
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

os=$(uname -s)
case "$os" in
Linux)
    ncpus=$(grep '^processor' /proc/cpuinfo | wc -l)
    ;;
Darwin)
    ncpus=$(sysctl -n hw.ncpu)
    ;;
*)
    echo "$0: cannot determine available parallelism on this platform" 1>&2
    ncpus=1
    ;;
esac
i=1
while [ $i -le $ncpus ] ; do
    echo -n "$i "
    i=$[ i * 2 ]
done
echo ""

