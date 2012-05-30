#!/usr/bin/perl

use strict;
use warnings;
use POSIX;

my $cwd = getcwd();

while (<>)
{
    chomp;

    my $i = index($_, $cwd);
    if ($i >= 0)
    {
	substr($_, $i, length($cwd)) = '$PWD';
    }

    print "$_\n";
}
