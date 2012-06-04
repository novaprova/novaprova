#!/bin/bash

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
    msg "FAIL $TEST $TESTARGS"
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
    if [ -f $TEST-normalize.pl ] ; then
	perl $TEST-normalize.pl < $f
    elif [ -f $TEST-normalize.awk ] ; then
	awk -f $TEST-normalize.awk < $f
    else
	# Default normalization
	egrep '^(EVENT|PASS|FAIL|N/A|EXIT|\?\?\?) ' < $f |\
	    sed -e 's/process [0-9]\+/process %PID%/g'
    fi
}

function runtest
{
    [ -e $TEST-pre.sh ] && bash $TEST-pre.sh $TEST $TESTARGS
    ./$TEST $TESTARGS
    echo "EXIT $?"
    [ -e $TEST-post.sh ] && bash $TEST-post.sh $TEST $TESTARGS
}

if [ $verbose ] ; then
    VERBOSE=yes runtest 2>&1 | tee $ID.log
else
    runtest > $ID.log 2>&1
fi

if [ -f $ID.ee ] ; then
    # compare logged output against expected output
    normalize $ID.log | diff -u $ID.ee - || fail
else
    expstatus=0
    egrep '^(FAIL|EXIT) ' $ID.log |\
    while read w d ; do
	case $w in
	FAIL) expstatus=1 ;;
	EXIT) [ $d == $expstatus ] || fail
	esac
    done
fi

pass
