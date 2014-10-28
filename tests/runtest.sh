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

function fatal()
{
    echo "$0: $*" 1>&2
    exit 1
}

function msg()
{
    echo "=== $*"
}

function fail()
{
    local mm="$*"
    [ -n "$mm" ] && mm=" ($mm)"
    msg "FAIL $TEST $TESTARGS$mm"

    for a in $TEST $TESTARGS ; do
	if [ -e a$a-failed.sh ] ; then
	    [ $verbose ] && msg "running a$a-failed.sh $TEST $TESTARGS"
	    bash a$a-failed.sh $TEST $TESTARGS
	fi
    done

    exit 1
}

function pass()
{
    msg "PASS $TEST $TESTARGS"
    exit 0
}

verbose=
if [ "$VERBOSE" = yes ] ; then
    verbose=yes
fi

TEST="$1"
[ -x $TEST ] || fatal "$TEST: No such executable"
shift
TESTARGS="$*"

ID="$TEST"
[ -n "$TESTARGS" ] && ID="$ID."$(echo "$TESTARGS"|tr ' ' '.')

[ $verbose ] && msg "starting $TEST $TESTARGS"

function normalize
{
    local f="$1"
    shift
    if [ -f $TEST-normalize.pl ] ; then
	perl $TEST-normalize.pl "$@" < $f
    elif [ -f $TEST-normalize.awk ] ; then
	awk -f $TEST-normalize.awk < $f
    else
	# Default normalization
	egrep '^(EVENT|MSG|PASS|FAIL|N/A|EXIT|\?\?\?) ' < $f |\
	    sed -r \
		-e 's|'$PWD'|%PWD%|g' \
		-e 's/process [0-9]+/process %PID%/g' \
		-e 's/0x[0-9A-F]{7,16}/%ADDR%/g'
    fi
}

function runtest
{
    for a in $TEST $TESTARGS ; do
	if [ -e a$a-pre.sh ] ; then
	    [ $verbose ] && msg "running a$a-pre.sh $TEST $TESTARGS"
	    bash a$a-pre.sh $TEST $TESTARGS
	fi
    done

    ./$TEST $TESTARGS
    echo "EXIT $?"

    for a in $TEST $TESTARGS ; do
	if [ -e a$a-post.sh ] ; then
	    [ $verbose ] && msg "running a$a-post.sh $TEST $TESTARGS"
	    bash a$a-post.sh $TEST $TESTARGS
	fi
    done
}

if [ $verbose ] ; then
    VERBOSE=yes runtest 2>&1 | tee $ID.log
else
    runtest > $ID.log 2>&1
fi

if [ -f $ID.ee ] ; then
    # compare logged output against expected output
    normalize $ID.log $TESTARGS | diff -u $ID.ee - || fail "differences to golden output"
else
    expstatus=0
    egrep '^(FAIL|EXIT) ' $ID.log > .logx
    exec < .logx
    while read w d ; do
	case $w in
	FAIL) expstatus=1 ;;
	EXIT)
	    if [ $d != $expstatus ] ; then
		fail "expecting exit status $expstatus got $d"
	    fi
	    if [ $d != 0 ] ; then
		fail "exit status $d"
	    fi
	    ;;
	esac
    done
fi

pass
