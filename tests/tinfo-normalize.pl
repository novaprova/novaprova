#!/usr/bin/perl

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
