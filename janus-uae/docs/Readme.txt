=========
Janus-UAE
=========

Janus-UAE is a fork of E-UAE 0.8.29-WIP4, so the same license(s)
apply to it. I changed the name, as I have different goals than
the e-uae developer. I only care for the AROS version and the
main idea of Janus-UAE was the integration into AROS as described
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

The bounty has been fulfilled and I want to thank all 
sponsors and the poeple behind power2people.org.

Sponsors:
Emmanuel L, Charles D, Ivano G, Clayton S, Graham L,
Alexander G, Dmitar B, Randy V, Gerd K, Martin H,
Mads-Jorgen S M, Gareth Keith B, Francis K, Drew H,
Frederic K, Robert C, Andreas H, Don C, David F, TA V,
Daniel H, Olivier A, Manuel A, Antonis I, Christopher P,
Nigel T, Robert W, Arnljot A, Paul B, George S, Troels E,
Ben M, Martin R, Matthias R, Marc L, Fabien I, Ralph B,
Brad M, Guillaume G, Fiona Bennett, Mike M, Georgios M,
Dag E S J, Margaret H, Albert C, Girish N, Nicolas M,
Chris F, Marcin W, Tibor E

============================================================
Installation:
============================================================
It should be enough, to copy the janus-uae file from
the aros directory in this archive to your
preferred location inside AROS. In the Amiga
directory you can find usefull applications
for AmigaOS (transdisk and transrom are not
built/tested by me). In the aros directory there
is a small binary to start AmigaOS applications.

AmigaOS helpers:
================

You have to start some AmigaOS executables (preferable
from s:user-startup) to use the integration features:

janusd
- mouse sync
- rootless mode (coherency)

clipd
- AmigaOS <-> AROS clipboard sync

launchd
- start AmigaOS workbench executables from wanderer
- run AmigaOS CLI executables with amirun

Best way is to run them at the end of your 
s:user-startup.
You can find janusd, clipd and launchd in the Amiga 
directory of this archive.

ATTENTION:
In order to test everything, it is a *good idea*,
to have coherency switched off during boot. If
everything works as expected, you can save
coherency in your uae config. If coherency is
on during boot, you can't see errors happening
until janusd is started. So if you get a
'please insert xy' requester before janusd
is run, you are lost.

If you get mouse trails, especially right after
starting AmigaOS 3.x, please replace the Picasso96
rtg.library with the one in this package.

AROS helpers:
================

amirun

With the AROS executable you can start any AmigaOS
executable, that is mounted somewhere in AmigaOS.

Usage: amirun <AmigaOS executable> [parameters]

amirun will exit with the following error codes,
if something goes wrong:

  121: j-uae/janusd not running
  103: out of AROS memory
  205: AmigaOS executable does not exist (in AROS filesystem)

At the moment, you get *no* return code from the AmigaOS side.
So you get *no* feedback, if the AmigaOS executeable is
within any mounted device, if it is an AmigaOS executeable
at all or if the AmigaOS executable returned any error code.
This might change in the future, but most likely not for
a v1.0.

All output always happens in an AmigaOS window, if there
is no output of the AmigaOS executable, no window is
opened. There is no way, to redirect the input/output
of the AmigaOS executeable.

Configuration:
==============
Janus-UAE uses a file uaerc.config whith the usual 
uae config syntax. 

You can show/hide the GUI at startup with use_gui=yes 
or use_gui=no. The GUI can also be shown/hidden
via exchange. The hotkey "ctrl alt j" toggles
GUI visibility.

There might be a problem, if you don't setup your sound
correctly. A linux music player may make the AmigaOS
boot hang (Krzysztof).

============================================================
History
============================================================

============================================================
J-UAE 1.2 (15.05.2012)
============================================================
There were some bug reports for v1.2, so this release
tries to fix some of them:

- J-UAE gui did not work well with updated Zune (commit 
  r44336). For example, all memory selections were 
  activated at the same time.
- Slider gadgets in the gui are now working (save/load).
- Opening the Amiga display on an own custom screen now
  does not open screen behind all other screens anymore.
- When started from CLI, CLI is not closed anymore
  after exit.

J-UAE is now built with i386-aros-gcc 4.2.4.

============================================================
J-UAE 1.1 (30.08.2011)
============================================================
New features:
- thanks to Krzysztof Smiechowicz, this is the first 
  version, which can run AROS/m68k
- added extended rom selection possibility in the gui
 
There were some bug reports for v1.0, so this release
tries to fix them, mainly:
- mouse pointer disappearance
  If you move the mouse pointer outside a CLI window, the
	mouse pointer disappeared in coherent mode.
- Gui drop box state was not saved/restored correctly.
- Startup output is STDOUT again.
- You can remove a selected Amiga rom key in the GUI.
- Amirun might work again.
- Fixed long time undetected memory trashing in the GUI.
- Stop J-UAE really stops J-UAE and does not restart 
  anymore.
- Pressing quit stops emulation before it quits.
- Double p96 id error requester does not appear anymore.
- 1920x1200 AmigaOS resolution should work (again)
- Load config did not really work.
 
============================================================
J-UAE 1.0 (21.04.2011)
============================================================
As the bounty has been completed, I bumped version number
to 1.0. This is just a clean non debug build of v0.9 with
no new features/bug fixes. I have added a "coherency-guide"
(PDF) in the "docs" folder of the archive, which tries
to give you a step-by-step guide to get coherence mode
up and running.

============================================================
J-UAE 0.9 (24.02.2011)
============================================================
I've made the following changes compared to J-UAE 0.8:

- GUI: configuration description is now displayed 
       and can be changed in About tab
- GUI: fixed p96 memory options, if 68000 is selected
- GUI: JIT buffer size maximum changed, uses MB instead of 
       bytes now (same as WinUAE)
- GUI: ROM image names are now updated correctly, if you
       load a new config file
- GUI: crash on exit after floppy image has been changed
       fixed 
- Quit with window gadget does not crash J-UAE any more
- possible AmigaOS library close race condition fixed.
- removed all debug and symbols, stripped executable
  has now the promised 9MB size (this is the first
  non debug build ever)

Known possible bugs:
====================
- choppy sound (won't fix that)
- format floppies does not work (might fix that)
- touch screen support (AROS should care for that?)
- pointer hide has problems in some configurations
  (hope to fix that)
- FinalWriter/FinalCalc problems (won't fix that?)

============================================================
J-UAE 0.8 (28.12.2010)
============================================================
There are way too many changes to the last version 0.7,
so please excuse the incomplete list. New:

- minimum blit size is increased to 64 bits whenever 
  possible to avoid noveau slowdowns
- window border gadgets are now done as AROS gadgets,
  no more ugly os3 window border gadgets, wherever possible.
- fixed a sound buffer overflow bug of e-uae
- fixed mouse pointer hide bug
- fixed many race conditions (I hope all of them)
- fixed mouse pointer trails bug (use supplied rtg.library)
- hotkey to show/hide GUI at any time
- load/save different configurations
- display current config file in about tab
- removed ugly gtk menu
- quit gui without running AmigaOS first, should 
  not crash anymore

Known Bugs:
===========
- This is a debug build, expect stripped version to be around 
  9MB in size.

============================================================
J-UAE 0.7 (27.04.2010)
============================================================
I've made the following changes compared to J-UAE 0.6:

- public screens are working again
- mouse pointer gets hidden (if AROS supports it)
- MUI menus work now
- popup menus done via real intuition windows work now
- fixed garbage window, if toggled integration
- added more resolutions
- destroyed uae window content is now restored correctly
- uae integration now works, if you use assigns as hardfiles
- 68k apps should be able to start from amidock etc.
- added utility, to start AmigaOS CLI commands from AROS (amirun)
- it has been reported, that sound works now
- it has been reported, that joysticks work now

Known Bugs:
===========
- This is a version 0.7, so be warned, this list is 
  not complete
- This is a debug build, expect stripped version to be around 
  9MB in size.
- sometimes the mousepointer leaves trails at the beginning.
  I have no idea, why.
- some people report crashes on exit, especially, when the
  main window close button is used.
- There are 4 freemem warnings on exit.

============================================================
J-UAE 0.6 (25.02.2010)
============================================================
I've made the following changes compared to J-UAE 0.5:

- possibility to add more RAM (up to 1.5 TB) and more gfx mem
- fixed bug when hiding windows (appeared on native only)
- fixed some race conditions

Known Bugs:
===========
- This is a version 0.6, so be warned, this list is 
  not complete
- This is a debug build, expect stripped version to be around 
  9MB in size.
- AmigaOS screen support is not finished (95% done). 
  Public screens won't work, custom screens should. 
  In classic mode they work of course.
- sometimes the mousepointer leaves trails at the beginning.
  I have no idea, why.
- No sound.

============================================================
J-UAE 0.5 (08.02.2010)
============================================================
I've made the following changes compared to J-UAE 0.4:

- GUI tab integration updated
- 68k AmigaOS executables can be started from WB
- custom screens are working

============================================================
J-UAE 0.4 (28.10.2009)
============================================================
I've made the following changes compared to J-UAE 0.3:

- new GUI tab "Integration" for janus special features
- clipboard can now be shared
- mouse can now be synced
- janus daemon has experimental custom screen support
- AmigaOS screens are not draggable, if janusd is running
- lots of bugfixes

Known Bugs:
===========
- This is a version 0.4, so be warned, this list is 
  not complete
- This is a debug build, expect stripped version to be around 
  9MB in size.
- AmigaOS screen support is not finished (80% done). 
  Public screens may work, custom screens not. 
  In classic mode they work of course.
- sometimes the mousepointer leaves trails at the beginning.
  I have no idea, why.
- No sound.

============================================================
J-UAE 0.3 (17.03.2009)
============================================================
I've made the following changes compared to J-UAE 0.2:

- picasso96 startup bug fixed
- full screen modes working
- new GUI tab "Display" replacing old "Chipset"
- GUI can be hidden (and shown again) via exchange
- exchange can quit J-UAE
- janus daemon has experimental public screen support
- bugfixes of course

The janus daemon is still in a very experimental state,
far away from beeing stable or complete.

Known Bugs:
===========
- This is a version 0.3, so be warned, this list is 
  not complete
- This is a debug build, expect stripped version to be around 
  9MB in size.

============================================================
J-UAE v0.2 (18.02.2009)
============================================================
I've made the following changes compared 
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

Sources:
========
J-UAE can be built with a GTK user interface for Linux. With
the help of GTK-MUI (v2), all requirements for a GTK-GUI for
AROS are met. To build J-UAE yourself with GTK for AROS,
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

If there are any bugs, feel free to report 
them to gtk-mui"-at-"oliver-brunner.de, remove the
part in "" obviously. 

o1i
