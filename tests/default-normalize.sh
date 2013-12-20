#!/bin/bash
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

# Default error log normalization
sed -r -e 's@^[a-z0-9]+\.(c|cxx):[0-9]+: (.*) \([^(]+\)$@EVENT \2@' |\
egrep '^(EVENT|MSG|PASS|FAIL|N/A|EXIT|np: WARNING:|\?\?\?) ' |\
    sed -r \
	-e 's|'$PWD'|%PWD%|g' \
	-e 's/process [0-9]+/process %PID%/g' \
	-e 's/0x[0-9A-F]{7,16}/%ADDR%/g'
