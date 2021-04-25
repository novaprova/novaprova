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

all:	run-tests

TESTS=	$(patsubst %.args,%,$(wildcard *.args))

run-tests:
	@nfailed=0 ; for test in $(TESTS) ; do \
	    res=FAIL ;\
	    ../ifdef.pl `cat $$test.args` < $$test.in > $$test.out 2>&1 ;\
	    diff -u $$test.out $$test.exp && res=PASS ;\
	    echo "$$res $$test" ;\
	    if [ $$res = FAIL ]; then nfailed=`expr $$nfailed + 1` ; fi ;\
	done ; echo "$$nfailed failures"
