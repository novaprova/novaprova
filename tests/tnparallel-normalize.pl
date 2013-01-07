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

my $expected_concurrency = 1;
my $o = shift;
if (defined $o)
{
    my ($e) = ($o =~ m/^-j(\d+)$/);
    $expected_concurrency = $e if defined $e;
}

my $maxn = 0;
my $n = 0;
my $startt = 0.0;
my $lastt = 0.0;
my $tott = 0.0;
my $first = 1;
my $epsilon = 0.1;
my @lines;

sub delta
{
    my ($ts, $d) = @_;

    if ($first)
    {
	$lastt = $startt = $ts;
	$first = 0;
    }

    $tott += $n * ($ts - $lastt);
    $lastt = $ts;
    $n += $d;
    $maxn = $n if ($n > $maxn);
}

while (<STDIN>)
{
    chomp;

    my ($ts) = m/^(\d+.\d+).* begins$/;
    if (defined $ts)
    {
	delta(0+$ts, 1);
	next;
    }

    ($ts) = m/^(\d+.\d+).* ends$/;
    if (defined $ts)
    {
	delta(0+$ts, -1);
	next;
    }

    my ($i) = m/^PASS tnparallel.(\d+)$/;
    if (defined $i)
    {
	$lines[$i-1] = $_;
	next;
    }

    if (m/^EXIT/)
    {
	foreach my $l (@lines)
	{
	    printf "$l\n" if defined $l;
	}
	print "$_\n";
	next;
    }
}

# printf "Maximum concurrency: %d\n", $maxn;

if ($maxn != $expected_concurrency)
{
    printf "FAIL expected maximum concurrency %d got %d\n",
	$expected_concurrency, $maxn;
}
else
{
    printf "PASS good maximum concurrency\n";
}

my $avg_conc = $tott/($lastt - $startt);
# printf "Average concurrency: %.2f\n", $avg_conc;

if ($avg_conc < (1-$epsilon)*$expected_concurrency ||
    $avg_conc > (1+$epsilon)*$expected_concurrency)
{
    printf "FAIL expected average concurrency between %.2f and %.2f got %.2f\n",
	(1-$epsilon)*$expected_concurrency,
	(1+$epsilon)*$expected_concurrency,
	$avg_conc;
}
else
{
    printf "PASS good average concurrency\n";
}
