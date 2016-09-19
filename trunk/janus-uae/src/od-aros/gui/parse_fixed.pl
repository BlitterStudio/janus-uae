#!/usr/bin/perl -w
#=====================================================================
#
# Script to parse a windows resource file and extract all strings
# defined there. It generates one big array in mui_data.cpp,
# which makes the rc-data easily available for the windows
# api-emulation functions.
#
# Copyright 2014-2016 Oliver Brunner - aros<at>oliver-brunner.de
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
use List::MoreUtils qw/ uniq /;
use File::Basename;

# TODO!!!!!!!!!!!!
# idc must be in the right order !!!
# TODO TODO

#=====================================================================
# global
#=====================================================================
my $debug=0;

# source
my $res_file=$ARGV[0];
# source
my $resh_file=$ARGV[1];
# cpp file to generate
my $cpp_file=$ARGV[2];
# header to generate
my $h_file  =$ARGV[3];

# source
local *RESFILE;
# source
local *RESHFILE;
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

# enum of IDC_'s
my @idc;

my $tree;

my $idd_name;
my $idd_active;

open(RESFILE, "<$res_file" ) or die "unable to open $res_file: $!";
open(RESHFILE, "<$resh_file" ) or die "unable to open $resh_file: $!";
open(CPPFILE, ">$cpp_file" ) or die "unable to open $cpp_file: $!";
if(!defined $h_file) {
  $h_file=$cpp_file;
  $h_file=~s/cpp$/h/;
}
open(HFILE  , ">$h_file"   ) or die "unable to open $h_file: $!";

print CPPFILE   "/* This file has been autogenerated with parse_fixed.pl from $res_file */ \n\n";
print CPPFILE   "#include <exec/types.h>\n";
print CPPFILE   "#include <libraries/mui.h>\n\n";
print CPPFILE   "#include \"sysconfig.h\"\n";
print CPPFILE   "#include \"sysdeps.h\"\n\n";
print CPPFILE   "#include \"options.h\"\n";
print CPPFILE   "#include \"gui.h\"\n\n";
print CPPFILE   "#include \"mui_data.h\"\n\n";

print HFILE   "/* This file has been autogenerated with parse_fixed.pl from $res_file */ \n\n";
my ($if_def_base, $if_def_dir, $if_def_suffix) = fileparse($h_file);
my $if_def=uc($if_def_base.$if_def_suffix);
$if_def=~s/\./_/;

print HFILE   "#ifndef __".$if_def."__\n";
print HFILE   "#define __".$if_def."__\n\n";
print HFILE   "#include \"sysconfig.h\"\n";
print HFILE   "#include \"sysdeps.h\"\n\n";
print HFILE   "#include \"gui_mui.h\"\n\n";

print CPPFILE "Element WIN_RES[] = {\n";

sub get_help($) {
  my $line=shift;

  $debug && print "get_help(".$line.")\n";

  if($line =~/\[\]/) {
    return "\"".(split/\[\]/,$line)[1];
  }
  return "NULL";
}

# respect minimal possible editable gadgets
# saller sizes crash AROS ..
sub min_height($) {
  my $h=shift;
  if($h<13) {
    return 13;
  }
  return $h
}

sub min_height_slider($) {
  my $h=shift;
  if($h<20) {
    return 20;
  }
  return $h
}

# strings may contain hot helps (mouse over help), splitted with [] from the button texts..
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
  #if($line eq "\"\"") {
    #$line="\" \"";
  #}

  #care for hotkeys: & => _
  $line=~s/&/_/g;
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
    elsif($a eq 'CBS_DROPDOWN') {
      $res|=0x2;
    }
    elsif($a eq 'CBS_DROPDOWNLIST') {
      $res=$res|=0x3;
    }
    elsif($a eq 'WS_GROUP') {
      # argl. This starts a group of autoradio buttons. Only one can be selected.. 
      $res|=0x00020000;
    }
    elsif($a eq 'WS_TABSTOP') {
      # used for TAB cycling. Maybe us this for MUI, too.
      $res|=0x00010000;
    }
    elsif($a eq 'WS_DISABLED') {
      $res|=0x08000000;
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

my $act_group=0;

# AUTORADIOBUTTONS need a group, so that only one option is enabled at the same time
# if WS_GROUP is detected, a new group starts
sub add_group($) {
  my $t=shift;

  if(not $t=~/BS_AUTORADIOBUTTON/) {
    return 0;
  }

  if($t=~/WS_GROUP/) {
    $act_group++;
    $debug && print "new group: ".$act_group."\n";
  }

  return $act_group;
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
      push @idc, $attr[1];
      printf(CPPFILE  "  { 0, %-20s, 0, NULL, NULL, NULL, %-11s, NULL, %3d, %3d, %3d, %3d, %s, %s, 0x%08lx, %d, 0 },\n",$attr[1] ,$type, $attr[2], $attr[3]+$plus_height, $attr[4], $attr[5], get_text($attr[0]), get_help($attr[0]), parse_flags($attr[6]), parse_flags2($attr[6]));
    }

    case "LTEXT" {
      $debug && print "  = RTEXT {\n";
      $debug && print "      string: ".$attr[0]."\n";
      $debug && print "      x:      ".$attr[2]."\n";
      $debug && print "      y:      ".$attr[3]."\n";
      $debug && print "      w:      ".$attr[4]."\n";
      $debug && print "      h:      ".$attr[5]."\n";
      $debug && print "    }\n";
      push @idc, $attr[1];
      printf(CPPFILE  "  { 0, %-20s, 0, NULL, NULL, NULL, %-11s, NULL, %3d, %3d, %3d, %3d, %s, %s, 0x%08lx, %d, 0 },\n", $attr[1], $type, $attr[2], $attr[3]+$plus_height, $attr[4], $attr[5], get_text($attr[0]), get_help($attr[0]), parse_flags($attr[6]), parse_flags2($attr[6]));
    }

    case "GROUPBOX" {
      $debug && print "  = GROUPBOX {\n";
      $debug && print "      string: ".$attr[0]."\n";
      $debug && print "      x:      ".$attr[2]."\n";
      $debug && print "      y:      ".$attr[3]."\n";
      $debug && print "      w:      ".$attr[4]."\n";
      $debug && print "      h:      ".$attr[5]."\n";
      $debug && print "    }\n";
      push @idc, $attr[1];
      printf(CPPFILE  "  { 0, %-20s, 0, NULL, NULL, NULL, %-11s, NULL, %3d, %3d, %3d, %3d, %s, %s, 0x%08lx, %d, 0 },\n", $attr[1], $type, $attr[2], $attr[3]+$plus_height, $attr[4], $attr[5], get_text($attr[0]), get_help($attr[0]), parse_flags($attr[6]), parse_flags2($attr[6]));
      #$node{'string'}=$attr[0];
      #$node{'x'}=$attr[2];
      #$node{'y'}=$attr[3];
      #$node{'w'}=$attr[4];
      #$node{'h'}=$attr[5];
    }
    case "CONTROL" {
      $debug && print "  = $type {\n";
      $debug && print "      string: ".$attr[0]."\n";
      $debug && print "      class:  ".$attr[2]."\n";
      $debug && print "      x:      ".$attr[4]."\n";
      $debug && print "      y:      ".$attr[5]."\n";
      $debug && print "      w:      ".$attr[6]."\n";
      $debug && print "      h:      ".$attr[7]."\n";
      $debug && print "    }\n";
      push @idc, $attr[1];
      if($attr[2] =~ /msctls_trackbar32/) {
        $attr[7]=min_height_slider($attr[7]);
      }
      printf(CPPFILE  "  { 0, %-20s, %d, NULL, NULL, NULL, %-11s, %-10s, %3d, %3d, %3d, %3d, %s, %s, 0x%08lx, %d, 0 },\n", $attr[1], add_group($attr[3]), $type, $attr[2], $attr[4], $attr[5]+$plus_height, $attr[6], $attr[7], get_text($attr[0]), get_help($attr[0]), parse_flags($attr[3]), parse_flags2($attr[3]));
    }
    case "PUSHBUTTON" {
      $debug && print "  = $type {\n";
      $debug && print "      string: ".$attr[0]."\n";
      $debug && print "      x:      ".$attr[2]."\n";
      $debug && print "      y:      ".$attr[3]."\n";
      $debug && print "      w:      ".$attr[4]."\n";
      $debug && print "      h:      ".$attr[5]."\n";
      $debug && print "    }\n";
      push @idc, $attr[1];
      printf(CPPFILE  "  { 0, %-20s, 0, NULL, NULL, NULL, %-11s, NULL, %3d, %3d, %3d, %3d, %s, %s, 0x%08lx, %d, 0 },\n", $attr[1], $type, $attr[2], $attr[3]+$plus_height, $attr[4], $attr[5], get_text($attr[0]), get_help($attr[0]), parse_flags($attr[6]), parse_flags2($attr[6]));
    }
    case "DEFPUSHBUTTON" {
      $debug && print "  = $type {\n";
      $debug && print "      string: ".$attr[0]."\n";
      $debug && print "      x:      ".$attr[2]."\n";
      $debug && print "      y:      ".$attr[3]."\n";
      $debug && print "      w:      ".$attr[4]."\n";
      $debug && print "      h:      ".$attr[5]."\n";
      $debug && print "    }\n";
      push @idc, $attr[1];
      printf(CPPFILE  "  { 0, %-20s, 0, NULL, NULL, NULL, %-11s, NULL, %3d, %3d, %3d, %3d, %s, %s, 0x%08lx, %d, 0 },\n", $attr[1], $type, $attr[2], $attr[3]+$plus_height, $attr[4], $attr[5], get_text($attr[0]), get_help($attr[0]), parse_flags($attr[6]), parse_flags2($attr[6]));
    }

    case "COMBOBOX" {
      $debug && print "  = $type {\n";
      $debug && print "      string: ".$attr[0]."\n";
      $debug && print "      x:      ".$attr[1]."\n";
      $debug && print "      y:      ".$attr[2]."\n";
      $debug && print "      w:      ".$attr[3]."\n";
      $debug && print "      h:      ".$attr[4]."\n"; # height is height of *opened* drop down box!!
      $debug && print "    }\n";
      push @idc, $attr[0];
      printf(CPPFILE  "  { 0, %-20s, 0, NULL, NULL, NULL, %-11s, NULL, %3d, %3d, %3d, %3d, %s, %s, 0x%08lx, %d, 0 },\n", $attr[0], $type, $attr[1], $attr[2]+$plus_height, $attr[3], 15, "\"".$attr[0]."\"", "NULL", parse_flags($attr[5]), parse_flags2($attr[5]));
    }
    case "EDITTEXT" {
      $debug && print "  = $type \n";
      $debug && print "      string: ".$attr[0]."\n";
      push @idc, $attr[0];
      printf(CPPFILE  "  { 0, %-20s, 0, NULL, NULL, NULL, %-11s, NULL, %3d, %3d, %3d, %3d, %s, %s, 0x%08lx, %d, 0 },\n", $attr[0], $type, $attr[1], $attr[2]+$plus_height, $attr[3], min_height($attr[4]), "\"".$attr[0]."\"", "NULL", parse_flags($attr[5]), parse_flags2($attr[5]));
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
    $act_group=0;
    my @t;
    # add some comment for better readability
    print CPPFILE "/* ".$line." */\n";
    $line=~s/\,//g; # remove all ,
    @t=split(' ', $line);
    $idd_name=$t[0];
    $debug && print "=== $idd_name ===\n";
    #print CPPFILE "Element ".$idd_name."[] = {\n";
    print CPPFILE "  { 0, ".$idd_name.", 0, NULL, NULL, NULL, DIALOGEX   , NULL,   ".$t[2].", ".$t[3].", ".$t[4].", ".$t[5].",  \"\", NULL, 0x10000000, 0, 0 },\n";
    push @idc, "oli was here\n"; 
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
        #print CPPFILE "  { 0, 0, 0, NULL, 0,            0,   0,   0,   0,  0, 0 }\n";
        #print CPPFILE "};\n\n";
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
print CPPFILE "  { 0, 0, 0, NULL, 0,            0,   0,   0,   0,  0, 0 }\n";
print CPPFILE "};\n";

# there are some IDC_ defines in resource.h, which are not in winuae.rc
# uniq() later cares for double entries
while(<RESHFILE>) {
  $line=$_;
  # strip all linefeeds
  $line=~s///;
  chop($line);
  if($line =~/IDC_/) {
    push @idc, (split(" ", $line))[1];
  }
}


if(0) {
print HFILE "extern struct Element *IDD_ELEMENT[];\n";

my $l;
foreach $l (@h_elements) {
  print HFILE "extern struct Element ".$l."[];\n";
}

print HFILE "\n";
print HFILE "/* Gadget identifiers */\n";
print HFILE "enum {\n";
print HFILE "  IDC_dummy,\n";
foreach $l (uniq(@idc)) {
  if($l =~ /^ID/) {
    print HFILE "  ".$l.",\n";
  }
}
print HFILE "};\n\n";

print CPPFILE "/* list of all pages */\n";
print CPPFILE "Element *IDD_ELEMENT[] = {\n";
foreach $l (@h_elements) {
  print CPPFILE "  ".$l.",\n";
}

print CPPFILE "  NULL,\n";
print CPPFILE "};\n\n";


}

close(RESFILE);
#=================
# get string stuff
#=================
open(RESFILE, "<$res_file" ) or die "unable to open $res_file: $!";

print HFILE "/* index values for STRINGTABLE array in mui_data.cpp */\n";
print HFILE "/* used by AROS .. */\n";
print HFILE "#undef IDS_DISABLED\n\n";
print HFILE "enum {\n";
print HFILE "  IDS_IDS_dummy,\n"; # IDC must not start with 0
print CPPFILE "/* STRINGTABLE contains all winuae.rc strings. For index values see mui_data.h */\n";
print CPPFILE "char *STRINGTABLE[] = {\n";
print CPPFILE "  \"THIS SHOULD NOT APPEAR!!\",\n";

my $stringtable_start=0;
my $stringtable_active=0;
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
  elsif($line =~ /STRINGTABLE/) {
    $stringtable_start=1;
  }
  else {
    if($stringtable_start) {
      #we are in a STRINGTABLE object, but not in the list of childs
      if($line =~ /^END/) {
        $stringtable_active=0;
        $stringtable_start=0;
      }
      else {
        if(!$stringtable_active and $line =~ /BEGIN/) {
          # list of childs start after BEGIN
          $debug && print "          .. active\n";
          $stringtable_active=1;
        }
        elsif($stringtable_active) {
          # IDS_ALWAYS_ON           "Always on"
          # ...
          $line=~s/^\s+//;
          @part=split(/\s+/, $line); # one or more white spaces
          $key=shift @part;
          $debug && print " == $key ==\n";
          $value=join ' ', @part;
          print HFILE "  $key,\n";
          print CPPFILE "  $value,\n";
        }

      }
    }
  }
}
print CPPFILE "};\n";

print HFILE "};\n\n";
close(RESHFILE);

# now copy windows resource.h to our own resource.h
# remove all strings, we already have them in mui_data.h
# attention: we need unique IDS_ values (at least they make our live easier)
my %ids;
my $p1;
my $p2;
my $max=0;
open(RESHFILE, "<$resh_file" ) or die "unable to open $resh_file: $!";
while(<RESHFILE>) {
  $line=$_;
  $line=~s/^M//;
  chop($line);
  if($line=~/^#define/ and not $line=~/IDS_/) {

    $p1=(split(/\s+/, $line))[1];
    $p2=(split(/\s+/, $line))[2];
    if(not defined $ids{$p2}) {
      $ids{$p2}=$p1;
    }
    else {
      # dup!
      $max=$p2+1;
      while(defined $ids{$max}) {
        $max++;
      }
      $ids{$max}=$p1; 
    }
  }
}
# print them nicely and sorted!
foreach $p2 (sort { $a <=> $b } keys %ids) {
    printf(HFILE "#define %-35s %5s\n",$ids{$p2}, $p2);
}

print HFILE   "\n#endif /* __".$if_def."__ */\n\n";

close(CPPFILE);
close(HFILE);
close(RESFILE);
close(RESHFILE);


