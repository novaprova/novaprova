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

TEST="$1"
[ -x $TEST ] || fatal "$TEST: No such executable"

msg "starting $TEST"
./$TEST
msg "$TEST returned $?"
