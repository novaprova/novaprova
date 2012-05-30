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
    msg "FAIL $TEST-$SUBTEST"
    exit 1;
}

verbose=
if [ "$1" = "--verbose" ] ; then
    verbose=yes
    shift
fi

TEST="$1"
[ -x $TEST ] || fatal "$TEST: No such executable"
SUBTEST="$2"
OBJDATA="d-$SUBTEST.o"
[ -f $OBJDATA ] || fatal "$OBJDATA: No such file"

[ $verbose ] && msg "starting $TEST-$SUBTEST"

function runtest
{
    ./$TEST $OBJDATA || fail
}

if [ $verbose ] ; then
    runtest 2>&1 | tee $TEST-$SUBTEST.log
else
    runtest > $TEST-$SUBTEST.log 2>&1
fi

if [ -f $TEST-$SUBTEST.ee ] ; then
    # compare events logged against expected events
    ./$TEST-normalize.pl $TEST-$SUBTEST.log | diff -u $TEST-$SUBTEST.ee - || fail
else
    echo "$TEST-$SUBTEST.ee: no such file or directory - create one!"
    fail
fi

msg "PASS $TEST-$SUBTEST"
exit 0
