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

/********************************
 * clone_menu
 *
 * take the menu tree from aos3
 * and add it to aros window
 ********************************/
ULONG *clone_menu(JanusWin *jwin) {
  struct Window   *aroswin=jwin->aroswin;
  uaecptr          aos3win=jwin->aos3win;
  struct Library  *GadToolsBase = NULL;
  struct NewMenu **newmenu;
  struct NewMenu  *m;
  struct Menu     *arosmenu;
  APTR             vi;
  uaecptr          aos3menustrip;

  JWLOG("started for jwin %lx\n", jwin);

  GadToolsBase = OpenLibrary( "gadtools.library", 37L );

  if(!aroswin || !aos3win) {
    JWLOG("aoswin or aos3win is NULL\n");
    return NULL;
  }

  if(!GadToolsBase) {
    JWLOG("GadToolsBase is NULL\n");
    return NULL;
  }

  /*The first argument is a pointer to an array of NewMenu structures */
  newmenu=AllocVec(5*sizeof(struct NewMenu *), MEMF_CLEAR);

#if 0
  m=AllocVec(sizeof(struct NewMenu), MEMF_CLEAR);
  m->nm_Type=NM_TITLE;
  m->nm_Label=g_strdup("Menu 1");
  newmenu[0]=m;

  m=AllocVec(sizeof(struct NewMenu), MEMF_CLEAR);
  m->nm_Type=NM_ITEM;
  m->nm_Label=g_strdup("Item 1-1");
  newmenu[1]=m;

  m=AllocVec(sizeof(struct NewMenu), MEMF_CLEAR);
  m->nm_Type=NM_ITEM;
  m->nm_Label=g_strdup("Item 1-2");
  newmenu[2]=m;

/* not necessary */
  m=AllocVec(sizeof(struct NewMenu), MEMF_CLEAR);
  m->nm_Type=NM_END;
  newmenu[3]=m;
#endif

  vi = GetVisualInfoA(jwin->jscreen->arosscreen, NULL);
  if(!vi) {
    JWLOG("ERROR: no vi (you have to use that editor!\n");
    return NULL;
  }

  aos3menustrip=get_long_p(aos3win + 28);
  if(!aos3menustrip) {
    JWLOG("aos3 window %lx has no menustrip\n",aos3win);
    return NULL;
  }
  JWLOG("aos3menustrip: %lx\n",aos3menustrip);

  m=AllocVec(sizeof(struct NewMenu), MEMF_CLEAR);
  m->nm_Type=NM_TITLE;
  m->nm_Label=get_real_address(get_long(aos3menustrip+14));
  JWLOG("adding menu %s\n",m->nm_Label);
  newmenu[0]=m;

  arosmenu=CreateMenus(*newmenu, NULL);
  if(!arosmenu) {
    JWLOG("failed to CreateMenus(%lx)\n",newmenu);
    return NULL;
  }

  if(!LayoutMenus(arosmenu, vi, GTMN_NewLookMenus, TRUE, TAG_DONE)) {
    JWLOG("LayoutMenus failed!\n");
    return NULL;
  }

  SetMenuStrip(aroswin, arosmenu);

  return arosmenu;
}


