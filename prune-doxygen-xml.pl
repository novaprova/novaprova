#!/usr/bin/perl
#
#  Copyright 2015-2021 Gregory Banks
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
use File::Find;
use XML::LibXML;
use File::Basename qw(basename);

my $me = basename($0);
my $basedir = shift
    or die "Usage: $0 basedir";

sub find_child
{
    my ($node, $child_name) = @_;

    # XML::LibXML's find() is terminally fucked.
    foreach my $child (@{$node->childNodes})
    {
	return $child
	    if ($child->nodeType == XML_ELEMENT_NODE &&
	        $child->nodeName eq $child_name)
    }
    return undef;
}

sub child_text
{
    my ($node, $child_name) = @_;
    my $child = find_child($node, $child_name);
    return $child->textContent if defined $child;
    return '';
}

sub has_brief_description
{
    my ($node) = @_;
    my $brief = find_child($node, 'briefdescription');
    return (!defined $brief || !$brief->hasChildNodes() || $brief->textContent() =~ /^\s*$/ ? 0 : 1);
}

my %filters = (
    innerclass => sub
    {
	my ($node) = @_;
	my $text = $node->textContent;
	return ($text, "in the std namespace") if $text =~ m/^std::/;
	return ($text, "name begins with __") if $text =~ m/^__/;
	return ($text, "no brief description") if !has_brief_description($node);
	return ($text);
    },
    memberdef => sub
    {
	my ($node) = @_;
	my $text = child_text($node, 'name');
	return ($text, "in the std namespace") if $text =~ m/^std::/;
	return ($text, "name begins with __") if $text =~ m/^__/;
	return ($text, "no brief description") if !has_brief_description($node);
	return ($text);
    }
);

sub filter_node
{
    my ($node) = @_;

    # silently accept anything that's not an element
    return 1 if ($node->nodeType != XML_ELEMENT_NODE);

    # Invoke an element-specific filter
    my $name = $node->nodeName;
    my $filter = $filters{$name};
    if (defined $filter)
    {
	my ($text, $reason) = $filter->($node);
	if (defined $reason)
	{
	    print "$me: rejecting $name $text ($reason)\n";
	    return 0;
	}
	else
	{
	    print "$me: accepting $name $text\n";
	    return 1;
	}
    }

    return 1;
}

sub walk_dom
{
    my ($node) = @_;

    return 0 if !filter_node($node);
    my @todie;
    foreach my $child (@{$node->childNodes})
    {
	push(@todie, $child) if !walk_dom($child);
    }
    foreach my $child (@todie)
    {
	$node->removeChild($child);
    }
    return 1;
}

my @files;
find(sub { push(@files, $File::Find::name) if m/\.xml$/ }, $basedir);
foreach my $file (@files)
{
    print "$me: pruning file $file\n";

    my $infh;
    open $infh,'<',$file
	or die "$me: Cannot open file $file for reading: $!";
    my $dom = XML::LibXML->load_xml(IO => $infh)
	or die "$me: Cannot parse $file as XML";
    close $infh;

    walk_dom($dom);

    my $newfile = $file . ".new";
    $dom->toFile($newfile, 0);

    rename($newfile, $file);
}


