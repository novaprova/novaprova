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

# build_and_test.sh has the full CI pipeline logic to build and test.
# In GitHub actions we don't use it as we break out each step
# for better error reporting.

function fatal()
{
    echo "$0: $*" 1>&2
    exit 1
}

image=
declare -a build_argv
build_argc=0
mode=build_and_test
# The working directory inside the container
workdir=/build/novaprova

function add_build_args()
{
    for a in "$@" ; do
        build_argv[$build_argc]="$a"
        build_argc=$[build_argc+1]
    done
}

function usage()
{
    echo "Usage: $0 [--compiler gcc|clang] [--enable-valgrind] image"
    exit 1
}

while [ $# -gt 0 ] ; do
    case "$1" in
    --compiler) add_build_args "$1" "$2" ; shift ;;
    --enable-valgrind|--disable-valgrind) add_build_args "$1" ;;
    --shell) mode=shell ;;
    --help) usage ;;
    -*) usage ;;
    *)
        [ -n "$image" ] && usage
        image="$1"
        ;;
    esac
    shift
done

case "$mode" in
build_and_test)
    set -ex
    docker run -v "$PWD:$workdir" -w $workdir $image \
        /bin/bash build/ci/build_and_test.sh "${build_argv[@]}"
    ;;
shell)
    set -ex
    container_id=$(docker create -v "$PWD:$workdir" -w $workdir $image tail -f /dev/null)
    docker start $container_id
    docker exec -w $workdir -it $container_id \
        /bin/bash build/ci/prepare-dev-environment.sh "${build_argv[@]}"
    docker exec -w $workdir -it $container_id /bin/bash
    docker rm -f $container_id
    ;;
esac

