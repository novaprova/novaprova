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

# prepare-dev-environment.sh has all the logic for preparing
# the development environment to be able to compile and test
# NovaProva, in a VM, a container, or a baremetal host.

host_os=$(uname -s)

function fatal()
{
    echo "$0: $*" 1>&2
    exit 1
}

function sudo()
{
    local binsudo=
    [ -x /bin/sudo ] && binsudo=/bin/sudo
    $binsudo "$@"
}

function setup_ubuntu()
{
    set -ex

    echo "Setting up development environment for Ubuntu"

    sudo apt-get update

    case "$compiler" in
    ""|gcc*)
        # Installing gcc may not also get us g++
        local compilers="gcc g++"
        ;;
    clang)
        local compilers="clang"
        ;;
    esac

    valgrind_package=
    [ "$valgrind" = yes ] && valgrind_package=valgrind

    apt-get install -y autoconf automake binutils-dev doxygen \
        libiberty-dev libxml-libxml-perl libxml2 libxml2-dev \
        libxml2-utils make pkg-config $valgrind_package \
        zlib1g-dev $compilers

    case "$compiler" in
    gcc*)
        # Make sure we have the HIGHEST installed gcc
        # version enabled as the default /usr/bin/gcc
        local gcc_version="$(ls /usr/bin/gcc* | sed -ne '/\/gcc-[0-9][0-9]*$/s/.*\/gcc-//p' | sort -nr | head -1)"
        if [ "$gcc_version" != "" ] ; then
            echo "Updating alternatives to gcc-${gcc_version} and g++-${gcc_version}"
            update-alternatives \
                --install /usr/bin/gcc gcc /usr/bin/gcc-${gcc_version} 60 \
                --slave /usr/bin/g++ g++ /usr/bin/g++-${gcc_version}
        fi
        ;;
    esac
}

function setup_redhat()
{
    set -ex

    echo "Setting up development environment for Fedora / RedHat"

    arch=x86_64
    case "$compiler" in
    ""|gcc*)
        # Installing gcc may not also get us g++
        local compilers="gcc g++"
        ;;
    clang)
        local compilers="clang.$arch"
        ;;
    esac

    valgrind_package=
    [ "$valgrind" = yes ] && valgrind_package=valgrind-devel.$arch

    sudo yum -y install \
        autoconf automake binutils-devel.$arch diffutils doxygen \
        libxml2.$arch libxml2-devel.$arch perl-XML-LibXML python-pip \
        strace $valgrind_package which $compilers

    sudo pip install breathe Sphinx
}

function setup_macos()
{
    set -ex

    echo "Setting up development environment for MacOS"

    brew tap novaprova/novaprova
    brew install autoconf automake binutils doxygen gettext libiberty libxml2
    # MacOS is clang only
    # Some releases of Valgrind work on MacOS, some don't, so
    # avoid cluttering up our CI results for now.
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

case "$host_os" in
Linux)
    if grep Ubuntu /etc/issue > /dev/null 2>&1 ; then
        setup_ubuntu
    elif [ -f /etc/redhat-release ] ; then
        setup_redhat
    else
        fatal "Unknown Linux distro"
    fi
    ;;
Darwin)
    setup_macos
    ;;
esac
