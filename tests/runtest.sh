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
    msg "FAIL $TEST"
    exit 1;
}

verbose=
if [ "$1" = "--verbose" ] ; then
    verbose=yes
    shift
fi

TEST="$1"
[ -x $TEST ] || fatal "$TEST: No such executable"

[ $verbose ] && msg "starting $TEST"

function runtest
{
    ./$TEST
    echo "EXIT $?"
}

if [ $verbose ] ; then
    runtest 2>&1 | tee $TEST.log
else
    runtest > $TEST.log 2>&1
fi

if [ -f $TEST.ee ] ; then
    # compare events logged against expected events
    egrep '^(EVENT|PASS|FAIL|EXIT) ' $TEST.log |\
	sed -e 's/process [0-9]\+/process %PID%/g' |\
	diff -u $TEST.ee - || fail
else
    expstatus=0
    egrep '^(FAIL|EXIT) ' $TEST.log |\
    while read w d ; do
	case $w in
	FAIL) expstatus=1 ;;
	EXIT) [ $d == $expstatus ] || fail
	esac
    done
fi

msg "PASS $TEST"
exit 0
