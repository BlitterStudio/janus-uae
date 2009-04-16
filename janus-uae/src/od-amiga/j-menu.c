/************************************************************************ 
 *
 * Copyright 2009 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-UAE.
 *
 * Janus-Daemon is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Janus-Daemon is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Janus-UAE. If not, see <http://www.gnu.org/licenses/>.
 *
 ************************************************************************/

#include <libraries/gadtools.h>
#include <proto/gadtools.h>
#include "j.h"

void init_item(struct NewMenu m, BYTE type, ULONG *text) {

  JWLOG("title: %s\n",text);

  m.nm_Type=type;
  m.nm_Label=text;

  return m;
}

/********************************
 * clone_menu
 *
 * take the menu tree from aos3
 * and add it to aros window
 ********************************/
void clone_menu(JanusWin *jwin) {
  struct Window   *aroswin=jwin->aroswin;
  uaecptr          aos3win=jwin->aos3win;
#warning Maximum amoun of menu items is set to 150 HARDCODED!!
  struct NewMenu   newmenu[150];
  struct NewMenu  *m;
  struct Menu     *arosmenu;
  APTR             vi;
  uaecptr          aos3menustrip;
  uaecptr          aos3item;
  uaecptr          aos3sub;
  uaecptr          aos3itemfill;
  ULONG            i;
  ULONG            nr_entries;

  JWLOG("started for jwin %lx\n", jwin);

  if(!aroswin || !aos3win) {
    JWLOG("aoswin or aos3win is NULL\n");
    return;
  }

  vi = GetVisualInfoA(jwin->jscreen->arosscreen, NULL);
  if(!vi) {
    JWLOG("ERROR: no vi (you have to use that editor!\n");
    return;
  }

  if(jwin->arosmenu) {
    /* we still have an old menu strip, we want to get rid of it (necessary?) */
    ClearMenuStrip(jwin->aroswin);
  }

  aos3menustrip=get_long_p(aos3win + 28);
  if(!aos3menustrip) {
    JWLOG("aos3 window %lx has no menustrip\n",aos3win);
    return;
  }
  JWLOG("aos3menustrip: %lx\n",aos3menustrip);

  i=0;
  while(aos3menustrip) {
    //init_item(newmenu[i],NM_TITLE, get_real_address(get_long(aos3menustrip+14)));
    newmenu[i].nm_Type=NM_TITLE;
    newmenu[i].nm_Label=get_real_address(get_long(aos3menustrip+14));
    newmenu[i].nm_CommKey=NULL;
    newmenu[i].nm_Flags=0;
    newmenu[i].nm_MutualExclude=NULL;
    newmenu[i].nm_UserData=666;

    i++;

    aos3item=get_long_p(aos3menustrip + 18);
    while(aos3item) {
      aos3itemfill=get_long(aos3item+18); /* IntuiText */
      if(aos3itemfill) { /* care for non intuitext stuff here ! */
	//init_item(newmenu[i],NM_ITEM, get_real_address(get_long(aos3itemfill+12)));
	newmenu[i].nm_Type =NM_ITEM;
	newmenu[i].nm_Label=get_real_address(get_long(aos3itemfill+12));
	//newmenu[i].nm_Flags=get_word(aos3item+12);
	newmenu[i].nm_CommKey=NULL;
	newmenu[i].nm_Flags=0;
	newmenu[i].nm_MutualExclude=NULL;
	newmenu[i].nm_UserData=666;
	i++;
      }
      aos3item=get_long(aos3item); /* NextItem */
    }

    aos3menustrip=get_long(aos3menustrip); /* NextMenu */
  }
//  m=clone_item(NM_TITLE, get_real_address(get_long(aos3menustrip+14)));
//  newmenu[0]=m;

  //init_item(newmenu[i], NM_END, NULL);
  newmenu[i].nm_Type=NM_END;
  newmenu[i].nm_Label=NULL;

  nr_entries=i;

  i=0;
  while(i < nr_entries) {
    JWLOG("newmenu[%d]: %d (%s)\n",i,newmenu[i].nm_Type,newmenu[i].nm_Label);
    i++;
  }

  /*The first argument is a pointer to an array of NewMenu structures */
  arosmenu=CreateMenus(newmenu, NULL);
  if(!arosmenu) {
    JWLOG("failed to CreateMenus(%lx)\n",newmenu);
    return;
  }

  if(!LayoutMenus(arosmenu, vi, GTMN_NewLookMenus, TRUE, TAG_DONE)) {
    JWLOG("LayoutMenus failed!\n");
    return;
  }

  SetMenuStrip(aroswin, arosmenu);

  jwin->arosmenu=arosmenu;

  return;
}


