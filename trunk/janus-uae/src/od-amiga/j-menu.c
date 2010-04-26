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

//#define JWTRACING_ENABLED 1
#include "j.h"

static ULONG  count_items(uaecptr menu);
static WORD   parse_flags(WORD aos3flags);
static STRPTR parse_label(UWORD flags, uaecptr intuitextptr);
static STRPTR parse_key(UWORD flags, uaecptr keytext);

/* gadtools uses inverted flags as intuition */
static WORD swap_enabled(WORD flags) {

  if(flags & ITEMENABLED) {
    flags=flags - ITEMENABLED;
  }
  else {
    flags=flags | ITEMENABLED;
  }
  return flags;
}

/* clear all non gadtools flags out of intuition flags */
static WORD parse_flags(WORD aos3flags) {
  WORD arosflags;

  JWLOG("aos3flags: %x\n",aos3flags);

  /* keep relevan flags */
  aos3flags=aos3flags & (CHECKIT | ITEMTEXT | COMMSEQ |
			 ITEMENABLED | CHECKED | HIGHNONE);

  arosflags=swap_enabled(aos3flags);

  JWLOG("arosflags: %x\n",arosflags);
  return arosflags;
}

/* return the label text pointer
 * we uses the original aos3 strings, no need to copy them
 *
 * a BIG problem here are MENUBARs. If you create them, they
 * are specified with 0xff as type. Once they are a real
 * MenuItem, ItemFill points to am image of the menubar, which we
 * can't detect (?). So we abuse HIGHNONE, as no "real" item
 * uses this anyways (hopefully)
 */
static STRPTR parse_label(UWORD flags, uaecptr intuitextptr) {

  if((flags & HIGHFLAGS)==HIGHNONE) {
    /* assume BAR */
    JWLOG("NM_BARLABEL\n");
    return NM_BARLABEL;
  }

  JWLOG("nm_Label: %s\n",get_real_address(get_long(intuitextptr)));
  return get_real_address(get_long(intuitextptr));
}

/* return command key or NULL*/
static STRPTR parse_key(UWORD flags, uaecptr keytext) {

  if(flags & COMMSEQ) {
    return get_real_address(keytext);
  }
  return NULL; /* if !NULL, AROS ignores !COMMSEQ ??*/
}

/* count required number of gadtools rows */
static ULONG count_items(uaecptr menu) {
  uaecptr item;
  uaecptr sub;
  ULONG i=0;

  while(menu) {
    i++;
    item=get_long(menu + 18);
    while(item) {
      i++;
      sub=get_long(item+28);
      while(sub) {
	i++;
	sub=get_long(sub);
      }
      item=get_long(item);
    }
    menu=get_long(menu);
  }

  i++; /* end tag */

  JWLOG("items counted: %d\n",i);
  return i;
}

/********************************
 * clone_menu
 *
 * take the menu tree from aos3
 * and add it to aros window
 ********************************/
void clone_menu(JanusWin *jwin) {
  struct Window   *aroswin=jwin->aroswin;
  uaecptr          aos3win=(uaecptr) jwin->aos3win;
  struct NewMenu  *newmenu;
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
    JWLOG("ERROR: no vi (you have to use that editor!)\n");
    return;
  }

  /* we still have an old menu strip, 
   * we want to get rid of it (necessary?) 
   */
  if(jwin->arosmenu) {
    ClearMenuStrip(jwin->aroswin);
  }

  aos3menustrip=get_long(aos3win + 28);
  if(!aos3menustrip) {
    JWLOG("aos3 window %lx has no menustrip\n",aos3win);
    return;
  }
  JWLOG("aos3menustrip: %lx\n",aos3menustrip);

  nr_entries=count_items(aos3menustrip);

  if(nr_entries != jwin->nr_menu_items) {
    /* we need other mem */
    FreeVecPooled(jwin->mempool, jwin->newmenu_mem);
    jwin->newmenu_mem=AllocVecPooled(jwin->mempool, 
                                     nr_entries * sizeof(struct Menu));
  }
  newmenu=jwin->newmenu_mem;

  i=0;
  while(aos3menustrip) {
    /*********** Menu ************/
    newmenu[i].nm_Type=NM_TITLE;
    newmenu[i].nm_Label=get_real_address(get_long(aos3menustrip+14));
    newmenu[i].nm_CommKey=NULL;
    /* Menu has other flags than MenuItem */
    if(get_word(aos3menustrip+12) & MENUENABLED) {
      newmenu[i].nm_Flags=0;
    }
    else {
      newmenu[i].nm_Flags=MENUENABLED;
    }
    newmenu[i].nm_MutualExclude=0;
    newmenu[i].nm_UserData=NULL;
    i++;

    aos3item=get_long(aos3menustrip + 18);
    while(aos3item) {
      /*********** item ************/
      aos3itemfill=get_long(aos3item+18); /* IntuiText */
      if(aos3itemfill) { 
	newmenu[i].nm_Type =NM_ITEM;
	newmenu[i].nm_Flags=parse_flags(get_word(aos3item+12)); 

	newmenu[i].nm_Label=parse_label(newmenu[i].nm_Flags, aos3itemfill+12);

	newmenu[i].nm_CommKey=parse_key(newmenu[i].nm_Flags, aos3item+26);
	newmenu[i].nm_MutualExclude=0;
	newmenu[i].nm_UserData=NULL;
	i++;

	aos3sub=get_long(aos3item+28);
	while(aos3sub) {
	  /*********** subitem ************/
	  aos3itemfill=get_long(aos3sub+18); /* IntuiText */
	  if(aos3itemfill) {
	    newmenu[i].nm_Type =NM_SUB;
	    newmenu[i].nm_Flags=parse_flags(get_word(aos3sub+12)); 
	    newmenu[i].nm_Label=parse_label(newmenu[i].nm_Flags, aos3itemfill+12);
	    newmenu[i].nm_CommKey=parse_key(newmenu[i].nm_Flags, aos3sub+26);
	    newmenu[i].nm_MutualExclude=0;
	    newmenu[i].nm_UserData=NULL;
	    i++;
	  }
	  aos3sub=get_long(aos3sub); /* Next */
	}
      }
      else {
	/* ignore */
	JWLOG("TODO: !aos3itemfill <==================\n");
      }
      aos3item=get_long(aos3item); /* NextItem */
    }

    aos3menustrip=get_long(aos3menustrip); /* NextMenu */
  }

  newmenu[i].nm_Type=NM_END;
  newmenu[i].nm_Label=NULL;

  nr_entries=i;

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

/******************************
 * wait_menu_shown
 *
 * waits until the MIDRAWN flag
 * indicates, that aos3 
 * intuition finished showing
 * our menu.
 *
 * Timeout, if it did not open
 ******************************/
static BOOL wait_menu_shown(uaecptr menu_flags) {
  ULONG i;
  UWORD flags;

  i=0;
  while(i<20) {
    flags=get_word(menu_flags);
    if(flags & MIDRAWN) {
      JWLOG("menu drawn (%d)\n",i);
      return TRUE;
    }
    i++;
    Delay(5);
  }
  JWLOG("menu not drawn !!\n");
  return FALSE;
}

static BOOL wait_menuitem_shown(uaecptr menu_flags) {
  ULONG i;
  UWORD flags;

  i=0;
  while(i<20) {
    flags=get_word(menu_flags);
    if(flags & ISDRAWN) {
      JWLOG("menu item drawn (%d)\n",i);
      return TRUE;
    }
    i++;
    Delay(5);
  }
  JWLOG("menu item not drawn !!\n");
  return FALSE;
}

/*****************************************************************
 * wait_menu_mouse_pos
 *
 * as the janusd can not move the mouse at once, we have to wait
 * until the mouse is on position. Use Delay() to not busy wait.
 *****************************************************************/
static BOOL wait_menu_mouse_pos(JanusWin *jwin, WORD MouseX, WORD MouseY) {
  ULONG i;
  uaecptr scr;

  scr=jwin->jscreen->aos3screen;
  i=0;
  while(i<20) {
    if((get_word(scr+16) == MouseY) &&
       (get_word(scr+18) == MouseX)) {
      JWLOG("mouse on position (%d)\n",i);
      return TRUE;
    }
    i++;
    Delay(5);
  }
  JWLOG("mouse not moved !!\n");
  return FALSE;
}

/* click_menu
 *
 * click on menu, item, subitem
 *
 * -1: don't use this level
 *
 * This is not what you would expect here. It is more Copperfield
 * Magic than amigaOS programming. We disabled all gfx updates
 * and locked out all mouse events from AROS. So now it is time
 * for the illusion. As nobody can see us, we move the amigaOS
 * mousepointer first to the menu, then to the item and if
 * necessary to the subitem. And then, we simply release the
 * right mouse button of the virtual uae mouse. voila ;).
 *
 * After that, we switch on mouse/updates again and nobody
 * has noticed it (hopefully ;)). Perfect illusion, or David?
 */
void click_menu(JanusWin *jwin, WORD menu, WORD item, WORD sub) {
  uaecptr          aos3win;
  uaecptr          aos3screen;
  uaecptr          aos3menustrip;
  uaecptr          aos3item;
  uaecptr          aos3sub;
  WORD             x, y;
  WORD             rx, ry;
  int              i,j,l;

  JWLOG("(%d, %d, %d)\n",menu, item, sub);

  aos3win=(uaecptr) jwin->aos3win;
  if(!aos3win) {
    return;
  }

  aos3menustrip=get_long(aos3win + 28);
  if(!aos3menustrip) {
    JWLOG("aos3 window %lx has no menustrip\n",aos3win);
    return;
  }

  aos3screen=(uaecptr) jwin->jscreen->aos3screen;

  i=0;
  rx=0;
  ry=0;
  while(aos3menustrip) {
    /*********** Menu ************/
    if(i==menu) {
  //    rx=get_word(aos3menustrip+4);
      JWLOG("menu left edge: %d\n",get_word(aos3menustrip+4));
      JWLOG("menu width:     %d\n",get_word(aos3menustrip+8));
      JWLOG("screen barh:    %d\n",get_byte(aos3screen + 30));

      x=get_word(aos3menustrip+4) +    /* LeftEdge */
	(get_word(aos3menustrip+8) /2);/* Width / 2 */
      /* "Currently", any values supplied for TopEdge and 
       * Height are ignored by Intuition, which uses instead 
       * the top of the screen for the TopEdge and the
       * height of the screen's title bar for the Height.
       * (Libraries Manual)
       */
      y=get_byte(aos3screen + 30) / 2; /* BarHeight */
      /* move to this menu */
      JWLOG("menu xy: %d %d\n\n",x,y); /* ok */
      menux=x;
      menuy=y;
      ry=get_byte(aos3screen + 30);
      ry=ry+1;
      rx=get_word(aos3menustrip+4);

      wait_menu_mouse_pos(jwin, menux, menuy);
      if(!wait_menu_shown(aos3menustrip + 12)) {
	JWLOG("menu not shown, do nothing\n");
	return;
      } 

      /* menu drawn */

      aos3item=get_long(aos3menustrip + 18);
      j=0;
      while((item!=-1) && aos3item) {
	if(j==item) {
	  JWLOG("menu item left edge: %d\n",get_word(aos3item+4));
	  JWLOG("menu item width:     %d\n",get_word(aos3item+8));
	  JWLOG("menu item top edge:  %d\n",get_word(aos3item+6));
	  JWLOG("menu item height:    %d\n",get_word(aos3item+10));

    	  x=get_word(aos3item+4) +    /* LeftEdge */
    	    get_word(aos3item+8) /2 + /* Width / 2 */
	    rx;                       /* The item LeftEdge is relative to the 
				       * LeftEdge of the Menu*/
       	  y=get_word(aos3item+6) +    /* TopEdge */
    	    get_word(aos3item+10) /2 +/* Height / 2 */
	    ry;
	  rx=rx+get_word(aos3item+4);
	  ry=ry+get_word(aos3item+6);
       	  /* move to this item */
	  JWLOG("menu item xy: %d %d\n\n",x,y);  /* ok */
	  menux=x;
	  menuy=y;
	  wait_menu_mouse_pos(jwin, menux, menuy);

	  /* if we have a subitem, we need to wait until it opens */
	  if(sub!=-1) {
	    if(!wait_menuitem_shown(aos3item + 12)) {
	      JWLOG("subitem not shown, ABORTED\n");
	      return;
	    };

	    aos3sub=get_long(aos3item+28);
	    l=0;
	    while(aos3sub) {
	      if(l==sub) {
		x=get_word(aos3sub+4) +    /* LeftEdge */
		  get_word(aos3sub+8) /2 + /* Width / 2 */
		  rx;
		y=get_word(aos3sub+6) +    /* TopEdge */
		  get_word(aos3sub+10) /2 +/* Height / 2 */
		  ry;
		/* move to this item */
		menux=x;
		menuy=y;
		JWLOG("menu subitem xy: %d %d\n",x,y);
		wait_menu_mouse_pos(jwin, menux, menuy);
	      }

	      l++;
	      aos3sub=get_long(aos3sub); /* Next */
	    }
	  }
	}
	j++;
  	aos3item=get_long(aos3item); /* NextItem */
      }
    }
    i++;
    aos3menustrip=get_long(aos3menustrip); /* NextMenu */
  }
}

void process_menu(JanusWin *jwin, UWORD selection) {
  WORD menu, item, sub;

  menu=MENUNUM(selection);
  item=ITEMNUM(selection);
  sub =SUBNUM (selection);

  JWLOG("menu %d, item %d, sub %d\n", menu, item , sub);

  if(menu == NOMENU || item == NOITEM) {
    /* nothing to do here */
    return;
  }

  if(sub == NOSUB) {
    sub=-1;
  }

  click_menu(jwin, menu, item, sub);
}
