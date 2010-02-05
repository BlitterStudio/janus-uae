/************************************************************************ 
 *
 * Copyright 2009-2010 Oliver Brunner - aros<at>oliver-brunner.de
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

#include "j.h"

/*********************************************************
 * j_reset
 *
 * AmigaOS was resetted, either throuh a crash or with the
 * Reset/Stop buttons.
 *
 * So we need to shutdown everything.
 *
 * The launch thread may continue to run, it is not
 * dependend on any AmigaOS daemon.
 ********************************************************/
void j_reset(void) {

  JWLOG("j_reset => unregister all daemons\n");

  /* reset janusd */
  close_all_janus_windows();
  close_all_janus_screens();
  aos3_task=NULL;
  aos3_task_signal=NULL;

  /* reset clipd */
  clipboard_hook_deinstall();
  aos3_clip_task=NULL;
  aos3_clip_signal=NULL;
  aos3_clip_to_amigaos_signal=NULL;

  /* reset launchd */
  aos3_launch_task=NULL;
  aos3_launch_signal=NULL;

  /* update gui */
  unlock_jgui();

  JWLOG("j_reset => done\n");
}

