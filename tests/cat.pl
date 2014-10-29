#!/usr/bin/perl

use strict;
use warnings;

my $s = 0;
my $n = 0;
while (<STDIN>)
{
    if (m/compile_unit {/)
    {
	$s = 1;
	next;
    }
    if ($s && m/} compile_unit/)
    {
	if ($n > 0)
	{
	    # elide empty compile unit
	    $n = 0;
	    print;
	}
	$s = 0;
	next;
    }
    if ($s && $n == 0)
    {
	print "compile_unit {\n";
    }
    $n += $s;
    print;
}
