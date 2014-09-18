/************************************************************************ 
 *
 * nogui.cpp
 *
 * No User Interface for the AROS version
 *
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

#include "sysconfig.h"
#include "sysdeps.h"

#include <intuition/intuition.h>
#include <proto/intuition.h>

/* used by AROS .. */
#undef IDS_DISABLED

#include "gui.h"
#include "string_resource.h"


/*
 * do nothing without GUI
 * TODO: if we have a GUI 
 */
int machdep_init (void) {
	return 1;
}

typedef struct {
  int idx;
  const char *msg;
} s_table;

static s_table transla[] = {
	NUMSG_NEEDEXT2, IDS_NUMSG_NEEDEXT2,
	NUMSG_NOROMKEY,IDS_NUMSG_NOROMKEY,
	NUMSG_NOROM,IDS_NUMSG_NOROM,
	NUMSG_KSROMCRCERROR,IDS_NUMSG_KSROMCRCERROR,
	NUMSG_KSROMREADERROR,IDS_NUMSG_KSROMREADERROR,
	NUMSG_NOEXTROM,IDS_NUMSG_NOEXTROM,
	NUMSG_MODRIP_NOTFOUND,IDS_NUMSG_MODRIP_NOTFOUND,
	NUMSG_MODRIP_FINISHED,IDS_NUMSG_MODRIP_FINISHED,
	NUMSG_MODRIP_SAVE,IDS_NUMSG_MODRIP_SAVE,
	NUMSG_KS68EC020,IDS_NUMSG_KS68EC020,
	NUMSG_KS68020,IDS_NUMSG_KS68020,
	NUMSG_KS68030,IDS_NUMSG_KS68030,
	NUMSG_ROMNEED,IDS_NUMSG_ROMNEED,
	NUMSG_EXPROMNEED,IDS_NUMSG_EXPROMNEED,
	//NUMSG_NOZLIB,IDS_NUMSG_NOZLIB,
	NUMSG_STATEHD,IDS_NUMSG_STATEHD,
	NUMSG_OLDCAPS, IDS_NUMSG_OLDCAPS,
	NUMSG_NOCAPS, IDS_NUMSG_NOCAPS,
	NUMSG_KICKREP, IDS_NUMSG_KICKREP,
	NUMSG_KICKREPNO, IDS_NUMSG_KICKREPNO,
	-1, NULL
};

static const char *gettranslation (int msg)
{
	int i = 0;
	while (transla[i].idx != -1) {
		if (transla[i].idx == msg)
			return transla[i].msg;
		i++;
	}
	return NULL;
}

void notify_user (int msg)
{
  struct EasyStruct req;
  const char *text;

  text=gettranslation(msg);
  if(!text) {
    printf("unknown message %d!!\n", msg);
    DebOut("unknown message %d!!\n", msg);
    return;
  }

  req.es_StructSize=sizeof(struct EasyStruct);
  req.es_TextFormat=text;
  req.es_Title="Janus-UAE";
  req.es_GadgetFormat="Ok";


  DebOut("notify_user(%d): %s \n", msg, text);
  EasyRequestArgs(NULL, &req, NULL, NULL );
}

void gui_display(int shortcut) {
  TODO();
}

void gui_flicker_led(int led, int unitnum, int status) {
  TODO();
}

void gui_disk_image_change (int unitnum, const TCHAR *name, bool writeprotected) {
  TODO();
}

void gui_gameport_axis_change (int port, int axis, int state, int max) {
  TODO();
}

void gui_gameport_button_change (int port, int button, int onoff) {
  TODO();
}

void gui_fps (int fps, int idle, int color) {
  TODO();
}
