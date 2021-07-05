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

scriptsdir=$(dirname $0)

set -ex
$scriptsdir/prepare-dev-environment.sh "$@"
$scriptsdir/configure.sh "$@"
make "$@"
make -k check "$@"

