#!/usr/bin/perl -w
#=====================================================================
# script to parse a windows resource file and extract all strings
# defined there.
#
# Copyright 2012       Oliver Brunner - aros<at>oliver-brunner.de
#
# This file is part of Janus-UAE.
#
# Janus-UAE is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Janus-UAE is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Janus-UAE. If not, see <http://www.gnu.org/licenses/>.
#
# $Id$
#
#=====================================================================


use strict;

if(!defined $ARGV[2]) {
  die "Usage: parse_res windows.rc out.cpp out.h"
}

my $res_file=$ARGV[0];
my $cpp_file=$ARGV[1];
my $h_file=$ARGV[2];

local *RESFILE;
local *CPPFILE;
local *HFILE;
my $line;
my @part;
my $active=0;
my $stringtable=0;
my $key;
my $value;


open(RESFILE,   $res_file ) or die "unable to open $res_file: $!";
open(CPPFILE, ">$cpp_file") or die "unable to open $cpp_file: $!";
open(HFILE  , ">$h_file"  ) or die "unable to open $h_file: $!";

while(<RESFILE>) {
  $line=$_;
  chomp($line);
  print "== $line\n";
  if($line =~ /^STRINGTABLE/) {
    $stringtable=1;
  }
  else {

    if($stringtable) {
      if($line =~ /^END/) {
        $active=0;
        $stringtable=0;
      }
      else {
        if(!$active and $line =~ /BEGIN/) {
          $active=1;
        }
        else {
          @part=split " ", $line;
          $key=shift @part;
          $value=join " ",@part;
          print CPPFILE "char ".$key."[]=".$value.";\n";
          print HFILE   "char *".$key.";\n";
        }

      }
    }
  }

}

close(RESFILE);
close(CPPFILE);
close(HFILE);

