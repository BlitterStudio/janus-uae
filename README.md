# janus-uae

Janus-UAE is a fork of E-UAE 0.8.29-WIP4, so the same license(s)
apply to it. I changed the name, as I have different goals than
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

Compilation:
============
These instructions require a Linux host. They have been tested on Ubuntu (even under WSL2) and Manjaro Linux.
Before you can compile Janus-UAE, you need to have completed the following:

### Requirements

- Read up on how to compile AROS in general, and install any requirements: http://www.aros.org/documentation/developers/compiling.php
- Clone the [AROS](https://github.com/aros-development-team/AROS) repository
- cd into the `AROS` dir and clone the [contrib](https://github.com/aros-development-team/contrib) and [ports](https://github.com/aros-development-team/ports) AROS repos as well (`libSDL` is in contrib).
- Restore all the git submodules: `git submodule update --init --recursive`
- Create a separate (sub)directory to build AROS into, e.g. `mkdir build` (for building inside `AROS/build`), and cd into it afterwards
- Ensure you have all the requirements installed, then run `../configure` with any relevant options (again, check the AROS docs)
- Build AROS: `make` - you can expect this to take a significant amount of time, even of fast systems.
- Create a symlink from `AROS/bin` to `AROS/build/bin` (or wherever your build directoy was), e.g. `cd .. && ln -s build/bin bin`
- Build `contrib` with `make contrib` (from within your `build` directory again!) - this will also take a long time.

### Compiling

- Clone this repo, by default the expected location is in `$HOME/projects`
- There are several Makefiles in the project that reference that path, if that's not the one you'll be using then you'll have to edit those paths manually.
- cd into the `src` directory and use `make x86_64` (for linux-x86_64). Option `i386` might not work (not tested)
- When the build is finished, you'll have the binary placed inside AROS' `SYS:` location, named `wuae`.

Installation:
====================================================
It should be enough, to copy the janus-uae file from
the aros directory in this archive to your
preferred location inside AROS. In the amiga
directory you can find usefull applications
for AmigaOS (transdisk and transrom are not
built/tested by me). In the aros directory there
is a small binary to start amigaOS applications.

amigaOS helpers:
================

You have to start some amigaos executables (preferable
from s:user-startup) to use the integration features:

janusd
- mouse sync
- rootless mode (coherency)

clipd
- amigaOS <-> AROS clipboard sync

launchd
- start amigaOS workbench executables from wanderer
- run amigaOS CLI executables with amirun

Best way is to run them at the end of your 
s:user-startup.
You can find janusd, clipd and launchd in the amiga 
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
starting amigaOS 3.x, please replace the Picasso96
rtg.library with the one in this package.

AROS helpers:
================

amirun

With the AROS executable you can start any amigaOS
executable, that is mounted somewhere in amigaOS.

Usage: amirun <amigaOS executable> [parameters]

amirun will exit with the following error codes,
if something goes wrong:

  121: j-uae/janusd not running
  103: out of AROS memory
  205: amigaOS executable does not exist (in AROS filesystem)

At the moment, you get *no* return code from the amigaOS side.
So you get *no* feedback, if the amigaOS executeable is
within any mounted device, if it is an amigaOS executeable
at all or if the amigaOS executable returned any error code.
This might change in the future, but most likely not for
a v1.0.

All output always happens in an amigaOS window, if there
is no output of the amigaOS executable, no window is
opened. There is no way, to redirect the input/output
of the amigaOS executeable.

Configuration:
==============
Janus-UAE uses a file uaerc.config with the usual
uae config syntax.

You can show/hide the GUI at startup with use_gui=yes
or use_gui=no. The GUI can also be shown/hidden
via exchange. The hotkey "ctrl alt j" toggles
GUI visibility.

History
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
- Quit with window gadget does not crash juae any more
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
- quit gui without running amigaos first, should 
  not crash anymore

Known Bugs:
===========
- This is a debug build, expect stripped version to be around 
  9MB in size.

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
- added utility, to start amigaOS CLI commands from AROS (amirun)
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
- amigaOS screen support is not finished (95% done). 
  Public screens won't work, custom screens should. 
  In classic mode they work of course.
- sometimes the mousepointer leaves trails at the beginning.
  I have no idea, why.
- No sound.

J-UAE 0.5 (08.02.2010)
============================================================
I've made the following changes compared to J-UAE 0.4:

- GUI tab integration updated
- 68k amigaOS executables can be started from WB
- custom screens are working

J-UAE 0.4 (28.10.2009)
============================================================
I've made the following changes compared to J-UAE 0.3:

- new GUI tab "Integration" for janus special features
- clipboard can now be shared
- mouse can now be synced
- janus daemon has experimental custom screen support
- amigaOS screens are not draggable, if janusd is running
- lots of bugfixes

Known Bugs:
===========
- This is a version 0.4, so be warned, this list is 
  not complete
- This is a debug build, expect stripped version to be around 
  9MB in size.
- amigaOS screen support is not finished (80% done). 
  Public screens may work, custom screens not. 
  In classic mode they work of course.
- sometimes the mousepointer leaves trails at the beginning.
  I have no idea, why.
- No sound.

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
