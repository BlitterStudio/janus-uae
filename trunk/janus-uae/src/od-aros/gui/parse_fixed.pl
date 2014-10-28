#!/usr/bin/perl -w
#=====================================================================
#
# script to parse a windows resource file and extract all strings
# defined there.
#
# Copyright 2014       Oliver Brunner - aros<at>oliver-brunner.de
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
use Switch;
require Tree::Simple;
use Data::TreeDumper;

#=====================================================================
# global
#=====================================================================
my $debug=0;

# source
my $res_file=$ARGV[0];
# cpp file to generate
my $cpp_file=$ARGV[1];
# header to generate
my $h_file  =$ARGV[2];

# source
local *RESFILE;
# cpp file to generate
local *CPPFILE;
# header to generate
local *HFILE;

my $line;
my @part;
my $active=0;
my $key;
my $value;
my $back="";
my $totalheight=320;
my $plus_height=0; # verticall centering

my $tree;

my $idd_name;
my $idd_active;

open(RESFILE, "<$res_file" ) or die "unable to open $res_file: $!";
open(CPPFILE, ">$cpp_file" ) or die "unable to open $cpp_file: $!";
if(!defined $h_file) {
  $h_file=$cpp_file;
  $h_file=~s/cpp$/h/;
}
open(HFILE  , ">$h_file"   ) or die "unable to open $h_file: $!";

print CPPFILE   "/* This file has been autogenerated with parse_gui.pl from $res_file */ \n\n";
print CPPFILE   "#include <exec/types.h>\n";
print CPPFILE   "#include <libraries/mui.h>\n\n";
print CPPFILE   "#include \"sysconfig.h\"\n";
print CPPFILE   "#include \"sysdeps.h\"\n\n";
print CPPFILE   "#include \"gui.h\"\n\n";

print HFILE   "/* This file has been autogenerated with parse_gui.pl from $res_file */ \n\n";
my $if_def=uc($h_file);
$if_def=~s/\./_/;

print HFILE   "#ifndef __".$if_def."__\n";
print HFILE   "#define __".$if_def."__\n\n";
print HFILE   "#include \"sysconfig.h\"\n";
print HFILE   "#include \"sysdeps.h\"\n\n";
print HFILE   "#include \"gui.h\"\n\n";

sub get_help($) {
  my $line=shift;

  $debug && print "get_help(".$line.")\n";

  if($line =~/\[\]/) {
    return "\"".(split/\[\]/,$line)[1];
  }
  return "NULL";
}

# strings my contain hot helps (mouse over help), splitted with [] from the button texts..
sub get_text($) {
  my $line=shift;
  my $t;

  if($line =~/\[\]/) {
    $debug && print "get_text:\n$line\n";
    $t=(split /\[\]/,$line)[0];
    $t=~s/\ $//g;
    $t=$t."\"";
    return $t;
  }
  return $line;
}

# parse flags and set the most important ones
sub parse_flags($) {
  my $t=shift;
  my @array;
  my $res =0x10000000;
  my $a;

  if(!defined $t) {
    return $res;
  }

  @array=split /\|/, $t;
  foreach $a (@array) {
    # trim both ends
    $a =~ s/^\s+|\s+$//g;
    #print ">$a<\n";
    if($a eq 'NOT WS_VISIBLE') {
      $debug && print "NOT WS_VISIBLE!\n";
      $res&= ~0x10000000;
    }
    elsif($a eq 'ES_READONLY') {
      $debug && print "ES_READONLY!\n";
      $res|= 0x0800;
    }
  }
  return $res;
}


sub parse_flags2($) {
  my $t=shift;
  my @array;
  my $res =0;
  my $a;

  if(!defined $t) {
    return $res;
  }

  @array=split /\|/, $t;
  foreach $a (@array) {
    # trim both ends
    $a =~ s/^\s+|\s+$//g;
    #print ">$a<\n";
    if($a eq 'BS_AUTOCHECKBOX') {
      $debug && print "BS_AUTOCHECKBOX!\n";
      $res=3;
    }
    elsif($a eq 'BS_AUTORADIOBUTTON') {
      $debug && print "BS_AUTORADIOBUTTON!\n";
      $res=9;
    }
  }
  return $res;
}


sub gen_line($$) {
  my $type=shift;
  my $args=shift;
  my $t;
  my $prev;
  my @attr;

  my @p=split(',', $args);

  # unescape lines with "... , ..." strings
  foreach $t (@p) {
    if(defined $prev) {
      $t=$prev.",".$t;
      undef $prev;
    }
    # line has only one "
    if(1 == $t =~ tr/"//) {
      $prev=$t;
      next;
    }
    push @attr, $t;
  }

  switch($type) {

    case "RTEXT" {
      $debug && print "  = RTEXT {\n";
      $debug && print "      string: ".$attr[0]."\n";
      $debug && print "      x:      ".$attr[2]."\n";
      $debug && print "      y:      ".$attr[3]."\n";
      $debug && print "      w:      ".$attr[4]."\n";
      $debug && print "      h:      ".$attr[5]."\n";
      $debug && print "    }\n";
      printf(CPPFILE  "  { 0, NULL, %-11s, %3d, %3d, %3d, %3d, %s, %s, 0x%08lx, %d },\n", $type, $attr[2], $attr[3]+$plus_height, $attr[4], $attr[5], get_text($attr[0]), get_help($attr[0]), parse_flags($attr[6]), parse_flags2($attr[6]));
    }

    case "LTEXT" {
      $debug && print "  = RTEXT {\n";
      $debug && print "      string: ".$attr[0]."\n";
      $debug && print "      x:      ".$attr[2]."\n";
      $debug && print "      y:      ".$attr[3]."\n";
      $debug && print "      w:      ".$attr[4]."\n";
      $debug && print "      h:      ".$attr[5]."\n";
      $debug && print "    }\n";
      printf(CPPFILE  "  { 0, NULL, %-11s, %3d, %3d, %3d, %3d, %s, %s, 0x%08lx, %d },\n", $type, $attr[2], $attr[3]+$plus_height, $attr[4], $attr[5], get_text($attr[0]), get_help($attr[0]), parse_flags($attr[6]), parse_flags2($attr[6]));
    }

    case "GROUPBOX" {
      $debug && print "  = GROUPBOX {\n";
      $debug && print "      string: ".$attr[0]."\n";
      $debug && print "      x:      ".$attr[2]."\n";
      $debug && print "      y:      ".$attr[3]."\n";
      $debug && print "      w:      ".$attr[4]."\n";
      $debug && print "      h:      ".$attr[5]."\n";
      $debug && print "    }\n";
      printf(CPPFILE  "  { 0, NULL, %-11s, %3d, %3d, %3d, %3d, %s, %s, 0x%08lx, %d },\n", $type, $attr[2], $attr[3]+$plus_height, $attr[4], $attr[5], get_text($attr[0]), get_help($attr[0]), parse_flags($attr[6]), parse_flags2($attr[6]));
      #$node{'string'}=$attr[0];
      #$node{'x'}=$attr[2];
      #$node{'y'}=$attr[3];
      #$node{'w'}=$attr[4];
      #$node{'h'}=$attr[5];
    }
    case "CONTROL" {
      $debug && print "  = $type {\n";
      $debug && print "      string: ".$attr[0]."\n";
      $debug && print "      x:      ".$attr[4]."\n";
      $debug && print "      y:      ".$attr[5]."\n";
      $debug && print "      w:      ".$attr[6]."\n";
      $debug && print "      h:      ".$attr[7]."\n";
      $debug && print "    }\n";
      printf(CPPFILE  "  { 0, NULL, %-11s, %3d, %3d, %3d, %3d, %s, %s, 0x%08lx, %d },\n", $type, $attr[4], $attr[5]+$plus_height, $attr[6], $attr[7], get_text($attr[0]), get_help($attr[0]), parse_flags($attr[3]), parse_flags2($attr[3]));
    }
    case "PUSHBUTTON" {
      $debug && print "  = $type {\n";
      $debug && print "      string: ".$attr[0]."\n";
      $debug && print "      x:      ".$attr[2]."\n";
      $debug && print "      y:      ".$attr[3]."\n";
      $debug && print "      w:      ".$attr[4]."\n";
      $debug && print "      h:      ".$attr[5]."\n";
      $debug && print "    }\n";
      printf(CPPFILE  "  { 0, NULL, %-11s, %3d, %3d, %3d, %3d, %s, %s, 0x%08lx, %d },\n", $type, $attr[2], $attr[3]+$plus_height, $attr[4], $attr[5], get_text($attr[0]), get_help($attr[0]), parse_flags($attr[6]), parse_flags2($attr[6]));
    }
    case "COMBOBOX" {
      $debug && print "  = $type {\n";
      $debug && print "      string: ".$attr[0]."\n";
      $debug && print "      x:      ".$attr[1]."\n";
      $debug && print "      y:      ".$attr[2]."\n";
      $debug && print "      w:      ".$attr[3]."\n";
      $debug && print "      h:      ".$attr[4]."\n"; # height is height of *opened* drop down box!!
      $debug && print "    }\n";
      printf(CPPFILE  "  { 0, NULL, %-11s, %3d, %3d, %3d, %3d, %s, %s, 0x%08lx, %d },\n", $type, $attr[1], $attr[2]+$plus_height, $attr[3], 15, "\"".$attr[0]."\"", "NULL", parse_flags($attr[5]), parse_flags2($attr[5]));
    }
    case "EDITTEXT" {
      $debug && print "  = $type \n";
      $debug && print "      string: ".$attr[0]."\n";
      printf(CPPFILE  "  { 0, NULL, %-11s, %3d, %3d, %3d, %3d, %s, %s, 0x%08lx, %d },\n", $type, $attr[1], $attr[2]+$plus_height, $attr[3], $attr[4], "\"".$attr[0]."\"", "NULL", parse_flags($attr[5]), parse_flags2($attr[5]));
    }
    else {
      $debug && print "      => default type\n";
    }
  }
  return;
}

#===================================================================0
# main
#===================================================================0
if(!defined $ARGV[1]) {
  die "Usage: parse_gui windows.rc out.cpp"
}

my @h_elements;

while(<RESFILE>) {
  $line=$back.$_;
  # strip all linefeeds
  $line=~s///;
  chop($line);
  $back="";
  #print ">$line<\n";
  if($line=~/,$/) {
    $back=$line;
    #print "BACK FOUND: $line\n";
    next;
  }
  chomp($line);
  $debug && print ("************************************************************************\n");
  $debug && print ("ACTUAL LINE: $line\n");
  $debug && print ("************************************************************************\n");
  if($line =~ /^;/) {
  }
  elsif($line =~ /DIALOGEX/) {
    # IDD_FLOPPY DIALOGEX 0, 0, 396, 261
    my @t;
    print CPPFILE "/* ".$line." */\n";
    $line=~s/\,//g; # remove all ,
    @t=split(' ', $line);
    $idd_name=$t[0];
    $debug && print "=== $idd_name ===\n";
    print CPPFILE "Element ".$idd_name."[] = {\n";
    $plus_height=($totalheight-$t[5])/2;
    push @h_elements, $idd_name;
    $idd_active=1;
    # shift away "IDD_FLOPPY DIALOGEX"
    shift(@t);
    shift(@t);
    #$debug && print "    x ".$root{'x'}." y ".$root{'y'}." w ".$root{'w'}." h ".$root{'h'}."\n";
  }
  else {
    if($idd_active) {
      #we are in a DIALOGEX object, but not in the list of childs
      if($line =~ /^END/) {
        $active=0;
        $idd_active=0;
        $debug && print ("=== END OF $idd_name ==\n");
        # finish!
        print CPPFILE "  { 0, NULL, 0,            0,   0,   0,   0,  NULL, 0 }\n";
        print CPPFILE "};\n\n";
      }
      else {
        if(!$active and $line =~ /BEGIN/) {
          # list of childs start after BEGIN
          $debug && print "          .. active\n";
          $active=1;
        }
        elsif($active) {
          # GROUPBOX        "Floppy Drives",IDC_SETTINGSTEXT3,1,0,393,163
          # COMBOBOX        IDC_DF1TYPE,152,51,65,50,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP
          # RTEXT           "Write-protected",IDC_STATIC,221,53,74,10,SS_CENTERIMAGE
          # ...
          @part=split " ", $line;
          $key=shift @part;
          $debug && print " == $key ==\n";
          $value=join " ",@part;
          # care for multilines?
          ##if(not $value =~ /"/) {
            # multiline 
            ##$back=$key." ";
          ##}
          ##else {
            # now we have a valid line
            $debug && print "      $value\n";
            gen_line($key, $value);
            #print CPPFILE "#define $key $value\n";
          ##}

        }

      }
    }
  }

}


close(RESFILE);
close(CPPFILE);
my $l;
foreach $l (@h_elements) {
  print HFILE "extern struct Element ".$l."[];\n";
}

print HFILE   "\n#endif /* __".$if_def."__ */\n\n";
close(HFILE);

