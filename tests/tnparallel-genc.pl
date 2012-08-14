#!/usr/bin/perl
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

