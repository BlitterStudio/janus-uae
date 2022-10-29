#!/usr/bin/perl -w
#=====================================================================
#
# script to parse a windows resource file and extract all strings
# defined there.
#
# Copyright 2015       Oliver Brunner - aros<at>oliver-brunner.de
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
#=====================================================================
use strict;

if(!defined $ARGV[1]) {
  die "Usage: parse_res windows.rc resource.h out.h"
}

my $res_file=$ARGV[0];
my $res_file2=$ARGV[1];
my $h_file=$ARGV[2];

local *RESFILE;
local *HFILE;
my $line;
my @part;
my $active=0;
my $stringtable=0;
my $key;
my $value;
my $back="";
my %pair;




sub parse_file($) {
  my $res=shift;

  open(RESFILE,   $res ) or die "unable to open $res: $!";
  while(<RESFILE>) {
    $line=$back.$_;
    $back="";
    chomp($line);
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
            if(not $value =~ /"/) {
              # multiline 
              $back=$key." ";
            }
            else {
              # don't patch IDS_CONTRIBUTOR lines
              if(not $key =~ /IDS_CONTRIBUTORS/) {
                $value=~s/WinUAE/Janus-UAE2/g;
              }
              $pair{$key}=$value;
              #print HFILE "#define $key $value\n";
            }

          }

        }
      }
    }

  }
  close(RESFILE);
}

parse_file($res_file);
parse_file($res_file2);

open(HFILE  , ">$h_file"  ) or die "unable to open $h_file: $!";
print HFILE   "/* This file has been autogenerated with parse_res.pl from $res_file and $res_file2 */ \n\n";
my $k;
foreach $k (keys %pair) {
  print HFILE "#define $k ".$pair{$k}."\n";
}
close(HFILE);
