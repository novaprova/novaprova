#!/usr/bin/perl

use strict;
use warnings;

while (<>)
{
    chomp;

    s/0x[0-9a-fA-F]{4,16}/0xXXX/;
    print "$_\n";
}
