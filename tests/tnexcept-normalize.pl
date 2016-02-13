#!/usr/bin/perl
#
#  Copyright 2011-2014 Gregory Banks
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

while (<>)
{
    chomp;

    # there are un-suppressable Valgrind errors under Linux
    next if m/VALGRIND.*unsuppressed errors found by valgrind/;

    next unless m/^(EVENT|MSG|PASS|FAIL|N\/A|EXIT|\?\?\?|np: WARNING:) /;
    print "$_\n";
}