#!/usr/bin/perl
#
#  Copyright 2011-2015 Gregory Banks
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
use POSIX;

my $cwd = getcwd();

# returns 1 iff the line in $_ should be accepted for output
sub norm_accept()
{
    return 1 if m/^EVENT /;
    return 1 if m/^MSG /;
    return 1 if m/^PASS /;
    return 1 if m/^FAIL /;
    return 1 if m/^N\/A /;
    return 1 if m/^EXIT /;
    return 1 if m/^np: WARNING:/;
    return 1 if m/^\?\?\? /;
    return 1 if m/^==\d+== [A-Z]/;
    return 0;
}

# Perform replacements on the line in $_ to make it
# sufficiently independent of the platform and runtime
# environment that it can survive a simple text comparison
# with the expected output in the .ee file.
sub norm_replace()
{
    while (1)
    {
	my $i = index($_, $cwd);
	last if ($i < 0);
	substr($_, $i, length($cwd)) = '$PWD';
    }

    s/process \d+/process %PID%/g;
    s/0x[0-9A-F]{7,16}/%ADDR%/g;
    s/^==\d+== /==%PID%== /g;
    s/size \d+/size %SIZE%/g;
    s/\d+ unsuppressed errors/%ERRORS% unsuppressed errors/g;
}

# Note: we ignore any arguments passed by the Makefile
while (<STDIN>)
{
    chomp;
    if (norm_accept())
    {
	norm_replace();
	print "$_\n";
    }
}

# vim
