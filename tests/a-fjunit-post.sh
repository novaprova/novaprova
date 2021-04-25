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

TEST="$1"

function fail()
{
    echo "FAIL $*"
    exit
}

[ -d reports ] || fail 'reports directory not created'
[ -f reports/TEST-$TEST.xml ] || fail 'report file not created'

xx=$(which xmllint 2>/dev/null)
[ -n "$xx" ] || fail 'could not find xmllint executable'

xmllint --schema JUnit.xsd -noout reports/TEST-$TEST.xml || \
    fail 'xml schema validation failed'
