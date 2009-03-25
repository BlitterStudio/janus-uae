=============
Janus-UAE 0.2
=============

Janus-UAE is a fork of E-UAE 0.8.29-WIP4, so the same license(s)
apply to it. I changed the name, as i have different goals than
the e-uae develper. I only care for the AROS version and the
main idea of Janus-UAE is the integration into AROS as described
in http://www.power2people.org/bounty_001.html:

UAE integration (basic/initial)

Objectives:

1. Must be able to run some classical software which 
   must include: WordWorth 6, Organizer 2, Datastore 2 
   and Money Matters 4.*
2. Must provide support for window and screen based apps 
   so they appear to be running on the host system - 
   including access to public screens.
3. Doubling-clicking a 68k application in Wanderer will 
   cause that application to be run in the emulation.
4. Each instance of emulation will be a commodity that can 
   be shut down via Exchange.
5. Port over a Zune based UAE prefs application.
6. UAE to use AROS clipboard.
7. separate directory (for 68K files) dictated by the 
   chosen config.

============================================================

I've made some changes the following changes compared 
to E-UAE 0.8.29-WIP4, mainly:

- compiling for GTK under AROS enabled
- slight changes to the used widgets
- small bugfixes
  + 24-bit mode can be saved for 68020
  + gtk optimized
- changed library open/close, as GTK-MUI normally expects,
  that a GTK program is a port from Unix and so does not
  open/close AROS Libraries.
- ZLib support (packed disk files, not tested)
- Picasso96 working
- the name ;)

Btw, does sound through AHI work? I have no idea about sound
on linux/amigaos (all my machines are silent, they have fans,
but no sound (besides beep)).

Known Bugs:
===========
- This is a version 0.2, so be warned, this list is 
  not complete
- Sometimes at startup, the Picasso96 screen is scrambled. I
  have no idea about the reason. Is not happening too often
  for me, so I am not hunting this bug ATM. Once it is up ok,
  it is ok.
- Hiding the GUI with exchange crashes the GUI. This is a 
  problem of the dummy implementation of the GDK functions
  of GTK-MUI.
- This is a debug build, expect stripped version to be around 
  9MB in size.

Bounty Status:
==============
1. Obviously completed ;)
2. No screen support.
3. Not done.
4. Shutdown not possible.
5. Done.
6. Not done.
7. Done (?).

Installation:
=============
It should be enough, to copy the janus-uae file from
the aros directory in this archive to your
preferred location inside AROS. In the amiga
directory you can find usefull applications
for AmigaOS (transdisk and transrom are not
built/tested by me).

Configuration:
==============
Janus-UAE uses a file uaerc.config whith the usual 
uae config syntax. You can enable/disable the GUI with
use_gui=yes or use_gui=no.

Sources:
========
E-UAE can be built with a GTK user interface for Linux. With
the help of GTK-MUI (v2), all requirements for a GTK-GUI for
AROS are met. To build E-UAE yourself with GTK for AROS,
you will need GTK-MUI from 
http://sourceforge.net/projects/gtk-mui/ and you will also 
need glib. Sources available at the same URL.
There are also pre-built libraries at
http://archives.aros-exec.org/share/development/library/libgtk-mui.i386.tar.gz
and
http://archives.aros-exec.org/share/development/library/glib.i386.tar.gz

You should be able to download the janus-uae sources 
from the same location, you've got the executable 
archive from. I've uploaded both packages to
http://archives.aros-exec.org/

The unmodified sources for E-UAE are available at:
http://sourceforge.net/projects/uaedev

Tools:
======
If you are missing some helper programs especially
for the AmigaOS side like mousehack or such, you
should be able to get them from the uaedev site.
I am not sure, which of those are useful/necessary.

License:
========
The GPL applies to all files (same as for E-UAE).
A copy of the GPL is supplied within this archive.

Disclaimer:
===========
I am no E-UAE developer (and don't want to become one),
I am doing Janus-UAE ;).

If there are any bugs in the GUI, feel free to report 
them to gtk-mui"-at-"oliver-brunner.de, remove the
part in "" obvioulsy. Bug reports for the Picasso96
support are welcome, too, of course.

18.02.2009
o1i
