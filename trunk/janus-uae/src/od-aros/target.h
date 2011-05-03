/************************************************************************ 
 *
 * Target specific stuff, AmigaOS version
 *
 * Copyright 1997 Bernd Schmidt
 * Copyright 2003-2006 Richard Drummond
 * Copyright 2011 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-UAE.
 *
 * Janus-UAE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Janus-UAE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Janus-UAE. If not, see <http://www.gnu.org/licenses/>.
 *
 * $Id$
 *
 ************************************************************************/

#define TARGET_AROS

#define TARGET_NAME		"AROS"

#define TARGET_ROM_PATH		"PROGDIR:Roms/"
#define TARGET_FLOPPY_PATH	"PROGDIR:Floppies/"
#define TARGET_HARDFILE_PATH	"PROGDIR:HardFiles/"
#define TARGET_SAVESTATE_PATH   "PROGDIR:SaveStates/"

#define UNSUPPORTED_OPTION_l

#define OPTIONSFILENAME "uaerc.config"
//#define OPTIONS_IN_HOME

#define TARGET_SPECIAL_OPTIONS \
    { "x",        "  -x           : Does not use dithering\n"}, \
    { "T",        "  -T           : Try to use grayscale\n"},
#define COLOR_MODE_HELP_STRING \
    "\nValid color modes (see -H) are:\n" \
    "     0 => 256 cols max on customscreen;\n" \
    "     1 => OpenWindow on default public screen;\n" \
    "     2 => Ask the user to select a screen mode with ASL requester;\n" \
    "     3 => use a 320x256 graffiti screen.\n\n"

#define DEFSERNAME "ser:"
#define DEFPRTNAME "par:"

#define PICASSO96_SUPPORTED

#define NO_MAIN_IN_MAIN_C
