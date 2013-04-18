#!/bin/bash
#
#  Copyright 2011-2013 Gregory Banks
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

package=novaprova
topdir=$(cd $(dirname $0)/.. ; /bin/pwd)

function fatal
{
    echo "$0: $*" 1>&2
    exit 1
}

function vdo
{
    echo "+ $*"
    "$@"
}

function copy_file
{
    local from="$1"
    local to="$2"

    echo "    $from -> $to"
    vdo cp $from $to || fatal "Cannot copy $from to $to"
}

# echo topdir=$topdir
# exit

vdo cd $topdir || fatal "Couldn't cd to $topdir"

echo "making a new debian/"
vdo /bin/rm -rf debian.old
[ -d debian ] && vdo mv debian debian.old
vdo mkdir debian

echo "copying files into debian/"
for f in build/obs/debian.* ; do
    copy_file $f debian/$(basename $f | sed -e 's|^debian\.||')
done

if [ -f debian/series ] ; then
    echo "setting up debian/patches/"
    vdo mkdir debian/patches
    mv debian/series debian/patches/series
    for f in $(awk '/^#/{next}{print $1}' debian/patches/series) ; do
	copy_file build/obs/$f debian/patches/$f
    done
fi

if [ -f debian/rules ] ; then
    vdo chmod +x debian/rules
    echo "adjusting debian/rules to install into a writable location"
    vdo perl -p -i -e 's|--destdir=\S+|--destdir=\$(CURDIR)/debian/'$package'|g' debian/rules

    if grep '^export DH_COMPAT=' debian/rules > /dev/null ; then
	echo "extracting debian/compat"
	awk -n -e 's/^export DH_COMPAT=([0-9]*)/\1/p' < debian/rules > debian/compat
    fi
fi

# It turns out that dpkg-buildpackage *HARDCODES* the location
# of where the results are uploaded to "..".  Bother.

vdo dpkg-buildpackage -b -uc -us || fatal "dpkg-buildpackage failed"

echo "moving build products to build/deb/"
[ -d build/deb ] || vdo mkdir build/deb
vdo mv -f ../$package*.deb ../$package*.changes build/deb
