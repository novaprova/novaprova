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

# Entry 0x25 [1] DW_TAG_namespace_type {
#     DW_AT_name = "foo"
#     DW_AT_decl_file = 0x1
#     DW_AT_decl_line = 0x3
#     DW_AT_sibling = (ref){0x0,0x3f}
# }

my %entries;
my $nexte = 1;

sub get_entry
{
    my ($off) = @_;
    my $e = $entries{$off};
    if (!defined $e)
    {
	$e = $entries{$off} = "E$nexte";
	$nexte++;
    }
    return $e;
}

my %ignored_tags = (
    producer => 1,
    comp_dir => 1,
    high_pc => 1,
    low_pc => 1,
    stmt_list => 1,
);

while (<>)
{
    chomp;

    my ($pre, $m, $post);

    if (($m) = m/DW_AT_([a-z_]+)/)
    {
	next if $ignored_tags{$m};
    }

    if (($pre, $m, $post) = m/^(Entry )(0x[[:xdigit:]]+)(.*)$/)
    {
	$_ = $pre . get_entry($m) . $post;
    }
    elsif (($pre, $m, $post) = m/^(.*\(ref\)\{0x[[:xdigit:]]+,)(0x[[:xdigit:]]+)(.*)$/)
    {
	$_ = $pre . get_entry($m) . $post;
    }

    print "$_\n";
}
