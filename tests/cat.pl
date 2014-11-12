#!/usr/bin/perl

use strict;
use warnings;

my $s = 0;
my $n = 0;
my $ignore = 0;
my $pending;
while (<STDIN>)
{
    if (m/^compile_unit /)
    {
	$s = 1;
	my ($dummy1, $path, $dummy2) = split;
	if (defined $path && $path ne '{' && ! -f $path)
	{
	    # a compile unit from libc or the C runtime
	    $ignore = 1;
	}
	else
	{
	    $ignore = 0;
	    $pending = $_;
	}
	next;
    }
    if ($s && m/} compile_unit/)
    {
	if ($n > 0)
	{
	    # elide empty compile unit
	    $n = 0;
	    print if (!$ignore);
	}
	$s = 0;
	next;
    }
    if ($s && $n == 0)
    {
	print "$pending" if (!$ignore);
	$pending = undef;
    }
    $n += $s;
    print if (!$ignore || m/^(EVENT|MSG|PASS|FAIL|N\/A|EXIT|\?\?\?)/);
}
