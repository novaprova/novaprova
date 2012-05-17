#!/usr/bin/perl

use strict;
use warnings;

my $N = 50;

print <<EOF;
#include <syslog.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "u4c.h"

EOF

foreach my $i (1..$N)
{
    my $d = int(1000000.0 * (0.25 + rand(0.75)));

    print <<EOF;
static void
test_$i(void)
{
//    fprintf(stderr, "%s test_$i begins\\n", u4c::reltimestamp().c_str()); fflush(stderr);
    usleep($d);
//    fprintf(stderr, "%s test_$i ends\\n", u4c::reltimestamp().c_str()); fflush(stderr);
}

EOF
}

print <<EOF;
int
main(int argc, char **argv)
{
    int ec = 0;
    u4c_runner_t *runner = u4c_init();
    u4c_set_concurrency(runner, /*maximal*/0);
    ec = u4c_run_tests(runner, 0);
    u4c_done(runner);
    exit(ec);
}
EOF
