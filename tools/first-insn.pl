#!/usr/bin/perl

use strict;
use warnings;

sub usage
{
    printf STDERR "Usage: $0 /lib/libc.so.6";
    exit 1;
}

my $dso = shift or usage;
my @cmd = ( 'objdump', '-d', '-j', '.text', $dso );
open DISASS, '-|', @cmd
    or die "Cannot run objdump to disassemble $dso: $!";

my $sym;
while (<DISASS>)
{
    chomp;
    if (m/<([a-zA-Z_][a-zA-Z_0-9]*)>:/)
    {
	$sym = $1;
	next;
    }
    if ($sym)
    {
	s/^\s*[0-9A-Fa-f]+:\s+//;
	while (m/^[0-9A-Fa-f_]+ [0-9A-Fa-f][0-9A-Fa-f]/)
	{
	    s/ /_/;
	}
	print "$_ $sym\n";
    }
    $sym = undef;
}

close DISASS;

