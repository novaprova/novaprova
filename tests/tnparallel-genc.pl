#!/usr/bin/perl

use strict;
use warnings;

my $N = 50;

print <<EOF;
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "np.h"

EOF

foreach my $i (1..$N)
{
    my $d = int(1000000.0 * (0.25 + rand(0.75)));

    print <<EOF;
static void
test_$i(void)
{
    fprintf(stderr, "%s test_$i begins\\n", np_rel_timestamp()); fflush(stderr);
    usleep($d);
    fprintf(stderr, "%s test_$i ends\\n", np_rel_timestamp()); fflush(stderr);
}

EOF
}

