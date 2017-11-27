=================
== Janus-UAE2  ==
=================

======================================================================
This is totally different from Janus-UAE 1.x. This is Janus-UAE2.
======================================================================

This is a non-feature-complete Alpha-Version (as defined in 
https://en.wikipedia.org/wiki/Software_release_life_cycle#Pre-alpha).

It is a direct port of WinUAE 3.2.0 (which you can download from
http://www.winuae.net/frames/download.html). It is *not* based
on e-uae or Janus-UAE 1.4.

So remember:

======================================================================
This is totally different from Janus-UAE 1.x. This is Janus-UAE2.
======================================================================

Why not a new name? Because the goal is, to integrate all the
functionality of Janus-UAE 1.4 into Janus-UAE2. But this means,
we first need a stable, after that I'll merge the other features.

What can this version do?
-------------------------

It offers the core functionality of WinUAE 3.2.0

This might (should) work:

- cpu emulation (with mmu)
- ocs/ecs/aga
- harddisk support (both hardfiles and host directories)
- the WinUAE gui, converted to Zune
- picasso96

Parts of the gui are hidden for this release, as they are not
complete/stable enough, even for an alpha-version ;).
All other parts like mouse sync, jit, sound or whatever 
are not yet done. Some parts might work, but are completely untested.

Bugs?
-----

For sure:
- You need enough memory. 1GB should be enough.
- Janus-UAE is a ABI v1 application. The backport to v0 might suffer from AROS
  bugs, which have been fixed in v1 but are not (yet?) backported to AROS v0
- Some Kickstarts seem to cause problems

======================================================================

You can contact me via aros@oliver-brunner.de.
From time to time I post progress reports on http://o1i.blogspot.de/

Janus-UAE2 is distributed under the same license as WinUAE, GPLv2,
which can be downloaded from:
http://www.gnu.org/licenses/gpl-2.0.html

This package contains no sources, but they are available at:
http://sourceforge.net/p/janus-uae/code/HEAD/tree/tags/

Alpha-Version sources can be downloaded at:
http://sourceforge.net/p/janus-uae/code/HEAD/tree/trunk/


Did I already mention this?

======================================================================
This is totally different from Janus-UAE 1.x. This is Janus-UAE2.
======================================================================

