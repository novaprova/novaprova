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
use Data::Dumper;

my @log;
my @open;
my $last = 0;

while (<>)
{
    # 6130305328 27331 3 begin setup_builtin_intercepts
    chomp;
    next if m/^#/;
    my ($ts, $pid, $depth, $which, $fn) = split;
# printf "XXX ts=%d depth=%d which=%s fn=%s\n", $ts, $depth, $which, $fn;

    if ($which eq 'begin')
    {
	my $l = {
	    depth => $depth,
	    func => $fn,
	    begin => 0+$ts,
	    parent => undef,
	    ncalls => 1,
	    children => []
	};
	if ($depth > 0)
	{
	    $l->{parent} = $open[$depth-1];
	    push(@{$l->{parent}->{children}}, $l);
	}
	$open[$depth] = $l;
	push(@log, $l);
    }
    else
    {
	my $l = $open[$depth];
	$open[$depth] = undef;
	$l->{end} = 0+$ts;
	$last = $l->{end} if ($last < $l->{end});
    }
# printf "XXX open=%s\n", Data::Dumper::Dumper(\@open);
}

sub assign_end
{
    my ($l) = @_;

    if (!defined $l->{end})
    {
	$l->{end} = (defined $l->{parent} ? assign_end($l->{parent}) : $last);
    }
    return $l->{end};
}

foreach my $l (@log)
{
    assign_end($l) if defined $l;
}


sub merge_children
{
    my ($l) = @_;

    my %seen;
    foreach my $child (@{$l->{children}})
    {
	my $orig = $seen{$child->{func}};
	if (!defined $orig)
	{
	    $seen{$child->{func}} = $child;
	}
	else
	{
	    $orig->{ncalls} += $child->{ncalls};
	    $child->{ncalls} = 0;
	    $orig->{end} += ($child->{end} - $child->{begin});
	    push(@{$orig->{children}}, @{$child->{children}});
	}
    }

    foreach my $child (@{$l->{children}})
    {
	next if !$l->{ncalls};
	merge_children($child);
    }

}

foreach my $l (@log)
{
    merge_children($l) if ($l->{depth} == 0);
}

foreach my $l (@log)
{
    printf "XXX log=%s\n", Data::Dumper::Dumper($l) if !defined $l->{begin} || !defined $l->{end};
}
# printf "XXX log=%s\n", Data::Dumper::Dumper(\@log);
# exit 0;

foreach my $l (@log)
{
    next if !$l->{ncalls};
    my $indent = '';
    for (my $i = 0 ; $i < $l->{depth} ; $i++)
    {
	$indent .= '    ';
    }
    my $elapsed = $l->{end} - $l->{begin};
    my $parent_elapsed = (defined $l->{parent} ? $l->{parent}->{end} - $l->{parent}->{begin} : $elapsed);
    printf("%s%s %d %d.%09d %.2f%%\n",
	    $indent, $l->{func},
	    $l->{ncalls},
	    int($elapsed / 1000000000),
	    $elapsed % 1000000000,
	    100.0 * ($elapsed / $parent_elapsed));
}
