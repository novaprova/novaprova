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

source ../plat.sh

FAILFILE=.failing-tests.dat

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

    # Add to the list of failing tests
    ( cat $FAILFILE 2>/dev/null ; echo "$TEST $TESTARGS" ) | LANG=C sort -u > $FAILFILE.new && mv $FAILFILE.new $FAILFILE

    exit 1
}

function pass()
{
    msg "PASS $TEST $TESTARGS"

    # Remove from the list of failing tests
    grep -v "^$TEST $TESTARGS\$" $FAILFILE 2>/dev/null > $FAILFILE.new && mv $FAILFILE.new $FAILFILE

    exit 0
}

function usagef()
{
    local msg="$*"
    [ -n "$msg" ] && echo "$0: $msg" 1>&2
    echo "Usage: $0 [--verbose] [--enable-valgrind] [--failing-only] test [arg...]" 1>&2
    exit 1
}

verbose=
enable_valgrind=no
failing_only=
TEST=
done=no

while [ $# -gt 0 -a $done = no ] ; do
    case "$1" in
    --verbose)
        verbose=yes
        export VERBOSE=yes
        msg "Enabling verbose output"
        ;;
    --enable-valgrind)
        enable_valgrind=yes
        ;;
    --failing-only)
        failing_only=yes
        ;;
    -*)
        usagef "Unknown option $1"
        ;;
    *)
        TEST="$1"
        shift
        TESTARGS="$*"
        [ -x $TEST ] || fatal "$TEST: No such executable"
        done=yes
        ;;
    esac
    shift
done
[ -z "$TEST" ] && usagef

ID="$TEST"
[ -n "$TESTARGS" ] && ID="$ID."$(echo "$TESTARGS"|tr ' ' '.')

[ $verbose ] && msg "starting $TEST $TESTARGS"

function normalize_output
{
    local f="$1"
    shift
    if [ -f $TEST-normalize.pl ] ; then
	perl $TEST-normalize.pl "$@" < $f
    elif [ -f $TEST-normalize.awk ] ; then
	awk -f $TEST-normalize.awk < $f
    else
	# Default normalization
	egrep '^(EVENT |MSG |PASS |FAIL |N/A |EXIT |np: .*\[(WARN|ERROR)\]|\?\?\? |==[0-9]+== [A-Z])' < $f |\
	    sed $sed_extended_opt \
                -e 's/\[(WARN|ERROR)\]/\1/' \
                -e 's/\[[^]=]*\]//g' \
		-e 's|'$PWD'|%PWD%|g' \
		-e 's/process [0-9]+/process %PID%/g' \
		-e 's/signal 15/signal %DIE%/g' \
		-e 's/0x[0-9A-F]{7,16}/%ADDR%/g' \
		-e 's/^==[0-9]+== /==%PID%== /g'
    fi
}

function normalize_golden_output()
{
    local f="$1"
    if [ $enable_valgrind = yes ] ; then
        cat "$f"
    else
        egrep -v '^==%PID%==' < "$f"
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

if [ $failing_only ]; then
    if ! grep "^$TEST $TESTARGS\$" $FAILFILE >/dev/null 2>&1 ; then
	exit 0
    fi
fi

if [ $verbose ] ; then
    VERBOSE=yes runtest 2>&1 | tee $ID.log
else
    runtest > $ID.log 2>&1
fi

if [ -f $ID.ee ] ; then
    # compare logged output against expected output
    normalize_output $ID.log $TESTARGS > $ID.log.norm
    if [ $verbose ] ; then
        msg "normalized output"
        cat $ID.log.norm
    fi
    normalize_golden_output $ID.ee > $ID.ee.norm
    if [ $verbose ] ; then
        msg "normalized golden output"
        cat $ID.ee.norm
    fi
    [ $verbose ] && msg "difference against golden output"
    diff -u $ID.ee.norm $ID.log.norm || fail "differences to golden output"
    rm -f $ID.ee.norm $ID.log.norm
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
