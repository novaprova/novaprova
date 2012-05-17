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
#include "np.h"

EOF

foreach my $i (1..$N)
{
    my $d = int(1000000.0 * (0.25 + rand(0.75)));

    print <<EOF;
static void
test_$i(void)
{
//    fprintf(stderr, "%s test_$i begins\\n", np::reltimestamp().c_str()); fflush(stderr);
    usleep($d);
//    fprintf(stderr, "%s test_$i ends\\n", np::reltimestamp().c_str()); fflush(stderr);
}

EOF
}

print <<EOF;
int
main(int argc, char **argv)
{
    int ec = 0;
    np_runner_t *runner = np_init();
    np_set_concurrency(runner, /*maximal*/0);
    ec = np_run_tests(runner, 0);
    np_done(runner);
    exit(ec);
}
EOF
