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
    local mm="$*"
    [ -n "$mm" ] && mm=" ($mm)"
    msg "FAIL $TEST $TESTARGS$mm"
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
	    sed -r \
		-e 's/process [0-9]+/process %PID%/g' \
		-e 's/0x[0-9A-F]{7,16}/%ADDR%/g'
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
    normalize $ID.log | diff -u $ID.ee - || fail "differences to golden output"
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
