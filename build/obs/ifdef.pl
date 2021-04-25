#!/usr/bin/perl
#
#  Copyright 2011-2021 Gregory Banks
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

sub usage
{
    printf STDERR "Usage: $0 [-Dvariable|-Dvariable=value ...] < infile > outfile";
    exit 1;
}

my %defines;
foreach my $a (@ARGV)
{
    if ($a =~ m/^-D/)
    {
        my @parts = split(/=/, substr($a, 2), 2);
        if (scalar(@parts) == 2)
        {
            my ($name, $value) = @parts;
            $defines{$name} = $value;
        }
        else
        {
            my ($name) = @parts;
            $defines{$name} = 1;
        }
        next;
    }
    usage;
}

my @stack;
while (<STDIN>)
{
    if (m/^\@ifdef\s+(\S+)\s*$/)
    {
        push(@stack, exists($defines{$1}));
        next;
    }
    if (m/^\@else\s*$/)
    {
        die '@else without @ifdef' unless scalar(@stack);
        $stack[-1] = !($stack[-1]);
        next;
    }
    if (m/^\@endif\s*$/)
    {
        die '@else without @ifdef' unless scalar(@stack);
        pop(@stack);
        next;
    }
    if (!scalar(@stack) || $stack[-1])
    {
        while (1)
        {
            if (m/\@([a-zA-Z_][a-zA-Z0-9_]*)\@/)
            {
                die "undefined variable $1" unless exists($defines{$1});
                $_ = $` . $defines{$1} . $';
                next;
            }
            last;
        }
        print;
    }

}
