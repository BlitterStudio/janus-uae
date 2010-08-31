/************************************************************************ 
 *
 * Copyright 2009 Oliver Brunner - aros<at>oliver-brunner.de
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

#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>

#define JWTRACING_ENABLED 1
#include "j.h"
#include "memory.h"

extern BOOL manual_mouse;
extern WORD manual_mouse_x;
extern WORD manual_mouse_y;

BOOL horiz_prop_active=FALSE;
BOOL vert_prop_active=FALSE;

/********************************************************
 * xy_compare
 *
 * compare two JanusGadgets, so that they are sorted 
 * according to their x (and second y) coordinates
 ********************************************************/
static gint xy_compare(gconstpointer a, gconstpointer b) {

  if( ((JanusGadget *)a)->x < ((JanusGadget *)b)->x) {
    return -1;
  }
  if( ((JanusGadget *)a)->x > ((JanusGadget *)b)->x) {
    return 1;
  }

  /* x == x, sort y */
  if( ((JanusGadget *)a)->y < ((JanusGadget *)b)->y) {
    return -1;
  }
  if( ((JanusGadget *)a)->y > ((JanusGadget *)b)->y) {
    return 1;
  }

  /* x == x and y == y ?? */
  return 0;
}

/********************************************************
 * change gadget
 *
 * if the janusgadget aos3 gadget is different from
 * the amigaos gadget, update/replace the 
 * janusgadget struct
 ********************************************************/
static ULONG change_gadget(JanusWin *jwin, ULONG type, ULONG gadget) {

  JWLOG("change_gadget(%lx, d, %lx)\n", jwin, type, gadget); 

  /* no new gadget ? */
  if(!gadget) {
    if(jwin->jgad[type]->aos3gadget) {
      /* free old JanusGadget struct */
      FreePooled(jwin->mempool, jwin->jgad[type], sizeof(JanusGadget));
      jwin->jgad[type]=NULL;
      return TRUE;
    }
    /* no new, no old, so it is ok */
    return FALSE;
  }

  /* new aos3 gadget ? */
  if(!jwin->jgad[type] || (jwin->jgad[type]->aos3gadget != gadget)) {

    if(jwin->jgad[type]) {
      /* free old janusGadget struct */
      FreePooled(jwin->mempool, jwin->jgad[type], sizeof(JanusGadget));
    }

    jwin->jgad[type]=(JanusGadget *) AllocPooled(jwin->mempool, sizeof(JanusGadget));

    jwin->jgad[type]->aos3gadget=gadget;

    return TRUE;
  }

  return FALSE;
}

/********************************************************
 * change jgadget
 *
 * if old an new differ, free old and link new
 ********************************************************/
static ULONG change_j_gadget(JanusWin *jwin, ULONG type, JanusGadget *new) {

  if(!jwin->jgad[type] && !new) {
    /* nothing to do */
    return FALSE;
  }

  if(!jwin->jgad[type]) {
    /* nothing to free here*/
    jwin->jgad[type]=new;
    /* but something changed of course */
    return TRUE;
  }

  /* compare old an new */
  if( jwin->jgad[type]->aos3gadget == new->aos3gadget ) {
    /* nothing to do, nothing changed */
    return FALSE;
  }

  FreePooled(jwin->mempool, jwin->jgad[type], sizeof(JanusGadget));
  jwin->jgad[type]=new;

  return TRUE;
}


/********************************************************
 * init_border_gadgets
 *
 * fill up/down/left/right arrow JanusGadgets
 ********************************************************/
ULONG init_border_gadgets(struct Process *thread, JanusWin *jwin) {
  ULONG gadget;
  ULONG specialinfo;
  UWORD gadget_type;
  UWORD gadget_flags;
  UWORD spezial_info_flags;
  WORD  x=0, y=0;
  JanusGadget *jgad;
  GList *aos3_gadget_list=NULL;
  ULONG changed=0;
  JanusGadget *left;
  JanusGadget *right;
  JanusGadget *up;
  JanusGadget *down;

  gadget=get_long_p(jwin->aos3win + 62);

  while(gadget) {

    gadget_type =get_word(gadget + 16); 
    gadget_flags=get_word(gadget + 12);

    if( ( !(gadget_type  & GTYP_SYSGADGET) ) &&
	( !(gadget_flags & GACT_TOPBORDER ) ) &&
	( !(gadget_flags & GACT_LEFTBORDER ) ) &&
        (  (gadget_type  & GTYP_CUSTOMGADGET) ) 
      ) {

      x=get_word(gadget + 4);
      y=get_word(gadget + 6);

      JWLOG("aros_win_thread[%lx]: cust gadget %lx: x %d y %d\n", thread, gadget, x, y);


      jgad=(JanusGadget *) AllocPooled(jwin->mempool, sizeof(JanusGadget));
      jgad->x=x;
      jgad->y=y;
      jgad->aos3gadget=gadget;

      aos3_gadget_list=g_list_append(aos3_gadget_list, (gpointer) jgad);
    }
    else if( gadget_type    & GTYP_PROPGADGET ) {

      specialinfo=get_long(gadget + 34);

      if(specialinfo) {
      	spezial_info_flags=get_word(specialinfo);
  	x=get_word(gadget + 4);
   	y=get_word(gadget + 6);

	if(spezial_info_flags & FREEHORIZ) {
 
	  JWLOG("aros_win_thread[%lx]: FREEHORIZ prop gadget %lx: x %d y %d\n", thread, gadget, x, y);

	  if(change_gadget(jwin, GAD_HORIZSCROLL , gadget)) {
	    //jgad=(JanusGadget *) AllocPooled(jwin->mempool, sizeof(JanusGadget));
	    jwin->jgad[GAD_HORIZSCROLL]->x=x;
	    jwin->jgad[GAD_HORIZSCROLL]->y=y;
	    jwin->jgad[GAD_HORIZSCROLL]->flags=get_word(specialinfo);
	    //jgad->aos3gadget=gadget;
	    //if(jwin->prop_left_right) {
	      //FreePooled(jwin->mempool, jwin->prop_left_right, sizeof(JanusGadget));
	    //}
	    //jwin->prop_left_right=jgad;
	    changed++;
	  }
	}

	if(spezial_info_flags & FREEVERT) {
	  //if(!jwin->prop_up_down || (jwin->prop_up_down->aos3gadget != gadget)) {
	  if(change_gadget(jwin, GAD_VERTSCROLL, gadget)) {
 
	    JWLOG("aros_win_thread[%lx]: FREEVERT prop gadget %lx: x %d y %d\n", thread, gadget, x, y);

	    //jgad=(JanusGadget *) AllocPooled(jwin->mempool, sizeof(JanusGadget));
	    jwin->jgad[GAD_VERTSCROLL]->x=x;
	    jwin->jgad[GAD_VERTSCROLL]->y=y;
	    jwin->jgad[GAD_VERTSCROLL]->flags=get_word(specialinfo);
	    //jgad->aos3gadget=gadget;
	    //if(jwin->prop_left_right) {
	      //FreePooled(jwin->mempool, jwin->prop_up_down, sizeof(JanusGadget));
	    //}
	    //jwin->prop_up_down=jgad;
	    changed++;
	  }
	}
      }
    }


    gadget=get_long(gadget); /* NextGadget */
  }

  aos3_gadget_list=g_list_sort(aos3_gadget_list, &xy_compare);

  if((g_list_length(aos3_gadget_list) == 4) || (g_list_length(aos3_gadget_list) == 2)) {
    /* otherwise don't even try */

    //if( change_j_gadget(jwin, &(jwin->arrow_left), (JanusGadget *) g_list_nth_data(aos3_gadget_list, 0));
    left  = (JanusGadget *) g_list_nth_data(aos3_gadget_list, 0);
    right = (JanusGadget *) g_list_nth_data(aos3_gadget_list, 1);

    if(!left || !right || (left->y != right->y)) {
      /* non-aligned, seems to be something different.. */
      changed += change_j_gadget(jwin, GAD_LEFTARROW,  NULL);
      // jwin->arrow_right=NULL;
      changed += change_j_gadget(jwin, GAD_RIGHTARROW, NULL);
    }
    else {
      changed += change_j_gadget(jwin, GAD_LEFTARROW,  left);
      changed += change_j_gadget(jwin, GAD_RIGHTARROW, right);
    }

    if(g_list_length(aos3_gadget_list) == 4) {
      //jwin->arrow_up   = (JanusGadget *) g_list_nth_data(aos3_gadget_list, 2);
      up   = (JanusGadget *) g_list_nth_data(aos3_gadget_list, 2);
      //jwin->arrow_down = (JanusGadget *) g_list_nth_data(aos3_gadget_list, 3);
      down = (JanusGadget *) g_list_nth_data(aos3_gadget_list, 3);
    }
    else {
      //jwin->arrow_up   = (JanusGadget *) g_list_nth_data(aos3_gadget_list, 0);
      up   = (JanusGadget *) g_list_nth_data(aos3_gadget_list, 0);
      //jwin->arrow_down = (JanusGadget *) g_list_nth_data(aos3_gadget_list, 1);
      down = (JanusGadget *) g_list_nth_data(aos3_gadget_list, 1);
    }

    //if(jwin->arrow_up->x != jwin->arrow_down->x) {
    if(up->x != down->x) {
      /* non-aligned, seems to be something different.. */
      changed += change_j_gadget(jwin, GAD_UPARROW,   NULL);
      changed += change_j_gadget(jwin, GAD_DOWNARROW, NULL);
    }
    else {
      changed += change_j_gadget(jwin, GAD_UPARROW,   up);
      changed += change_j_gadget(jwin, GAD_DOWNARROW, down);
    }
  }
  else {
    /* clear all old gadget, if there were any */
    changed += change_j_gadget(jwin, GAD_LEFTARROW,  NULL);
    changed += change_j_gadget(jwin, GAD_RIGHTARROW, NULL);
    changed += change_j_gadget(jwin, GAD_UPARROW,    NULL);
    changed += change_j_gadget(jwin, GAD_DOWNARROW,  NULL);
  }

  g_list_free(aos3_gadget_list);
  /* there still might be some elements alloced, in case !=2 and !=4 for example.
   * but they are from our pool, so we save the trouble of freeing them.
   */

  return changed;
}

/********************************************************************
 * move_horiz_prop_gadget(process, jwin)
 *
 * move the (amigaos) horizontal slider to the same position, as the
 * AROS slider is at the moment
 ********************************************************************/
void move_horiz_prop_gadget(struct Process *thread, JanusWin *jwin) {
  UWORD x;
  ULONG t;
  ULONG rulerwidth;
  ULONG freewidth;
  ULONG specialinfo;
  JanusGadget *jgad=NULL;

  jgad=jwin->jgad[GAD_HORIZSCROLL]; 
  specialinfo=get_long(jgad->aos3gadget + 34);

  /* place mouse on the horizontal amigaos proportional gadget */
  /* y middle of gadget: TopEdge + Height - BorderBottom/2 */
  manual_mouse_y=get_word_p(jwin->aos3win+6) + get_word_p(jwin->aos3win+10) - (get_byte((uaecptr) jwin->aos3win+57)/2);

  JWLOG("aros_win_thread[%lx]:  manual_mouse_y %d\n", thread, manual_mouse_y);

  JWLOG("aros_win_thread[%lx]:  width %d\n", thread, get_word(jgad->aos3gadget +  8));
  JWLOG("aros_win_thread[%lx]:  horizpot %x\n", thread, get_word(specialinfo + 2));
  JWLOG("aros_win_thread[%lx]:  CWidth %d\n", thread, get_word(specialinfo + 10));
  JWLOG("aros_win_thread[%lx]:  LeftBorder %d\n", thread, get_word(specialinfo + 18));

  /* left edge + left border width of aos3 window */
  x=get_word_p(jwin->aos3win + 4) + get_byte((uaecptr) jwin->aos3win + 54);

  /* size of ruler = 68k gadget width * HorizBody / MAXBODY */
  rulerwidth=get_word(specialinfo + 10) * get_word(specialinfo + 6) / MAXBODY;
  JWLOG("aros_win_thread[%lx]:  rulerwidth %d\n", thread, rulerwidth);

  /* leftmost x clickpoint for us
   * assuming the slider gadget is in it's leftmost position, we need to click in the middle of the
   * slider (rulerwidth / 2). We also add the border of the Propgadget, as we don't want to
   * click on that either.
   */
  x = x + rulerwidth/2 + get_word(specialinfo + 18) ;

  /* free space outside of ruler */
  freewidth=get_word(specialinfo + 10) - rulerwidth;

  /* we now take the percentage of the freewidth, that corresponds to the actual prop value */
  t=(freewidth * ((struct PropInfo *) jwin->gad[GAD_HORIZSCROLL]->SpecialInfo)->HorizPot) / MAXPOT;

  /* click point!
   * as we only use the freewidth value, the maximum we can reach is the middle of the
   * slider, if the slider is on the leftmost position. This is exactly, what we want.
   */
  x=x + t; 

  JWLOG("aros_win_thread[%lx]: GAD_HORIZSCROLL: x %d\n", thread, x);

  manual_mouse_x=x;
  /* debug! */

  JWLOG("aros_win_thread[%lx]: hit gadget %lx at %d x %d..\n", thread, jgad->aos3gadget, manual_mouse_x, manual_mouse_y);

  mice[0].enabled=FALSE; /* disable mouse emulation */
  manual_mouse=TRUE;
  while(manual_mouse) {
    /* wait until the mouse is, where it should be */
    Delay(1);
  }
  Delay(10);
}

/********************************************************************
 * move_vert_prop_gadget
 *
 * same for vertiacl slider
 ********************************************************************/
void move_vert_prop_gadget(struct Process *thread, JanusWin *jwin) {
  UWORD y;
  ULONG t;
  ULONG rulerheight;
  ULONG freeheight;
  ULONG specialinfo;
  JanusGadget *jgad=NULL;

  //jgad=jwin->prop_up_down; 
  jgad=jwin->jgad[GAD_VERTSCROLL]; 
  specialinfo=get_long(jgad->aos3gadget + 34);

  /* place mouse on the vertical amigaos proportional gadget */
  /* x middle of gadget: LeftEdge + Width - BorderRight/2 */
  manual_mouse_x=get_word_p(jwin->aos3win+4) + get_word_p(jwin->aos3win+8) - 
                 (get_byte((uaecptr) jwin->aos3win+56)/2);

  JWLOG("aros_win_thread[%lx]:  manual_mouse_x %d\n", thread, manual_mouse_x);

  JWLOG("aros_win_thread[%lx]:  window top edge %d\n", thread, get_word_p(jwin->aos3win + 6));
  JWLOG("aros_win_thread[%lx]:  window top border height %d\n", thread, get_byte((uaecptr) jwin->aos3win + 55));
  JWLOG("aros_win_thread[%lx]:  gadget specialinfo CHight %d\n", thread, get_word(specialinfo + 12));
  JWLOG("aros_win_thread[%lx]:  aros VertPot %d\n", thread, ((struct PropInfo *) jwin->gad[GAD_VERTSCROLL]->SpecialInfo)->VertPot);

  /* top edge + top border height of aos3 window */
  y=get_word_p(jwin->aos3win + 6) + get_byte((uaecptr) jwin->aos3win + 55);

  /* size of ruler = 68k gadget height * VertBody / MAXBODY */
  rulerheight=get_word(specialinfo + 12) * get_word(specialinfo + 8) / MAXBODY;
  JWLOG("aros_win_thread[%lx]:  rulerheight %d\n", thread, rulerheight);

  /* topmost y clickpoint for us
   * assuming the slder gadget is in it's top position, we need to click in the middle of the
   * slider (rulerheight / 2). We also add the border of the Propgadget, as we don't want to
   * click on that either.
   */
  y=y + rulerheight/2 + get_word(specialinfo + 20) ;

  /* free space outside of ruler */
  freeheight=get_word(specialinfo + 12) - rulerheight;

  /* we now take the percentage of the freeheight, that corresponds to the actual prop value */
  t=(freeheight * ((struct PropInfo *) jwin->gad[GAD_VERTSCROLL]->SpecialInfo)->VertPot) / MAXPOT;
  JWLOG("aros_win_thread[%lx]:  t %d\n", thread, t);

  /* click point!
   * as we only use the freeheight value, the maximum we can reach is the middle of the
   * slider, if the slider is on the lowest position. This is exactly, what we want.
   */
  y=y + t; 

#if 0
  /* otherwise we will miss the gadget */
  if(y<2) {
    y=2;
  }
#endif

  JWLOG("aros_win_thread[%lx]: GAD_VERTSCROLL: y %d\n", thread, y);

  manual_mouse_y=y;

  JWLOG("aros_win_thread[%lx]: hit gadget %lx at %d x %d..\n", thread, jgad->aos3gadget, manual_mouse_x, manual_mouse_y);

  mice[0].enabled=FALSE; /* disable mouse emulation */
  manual_mouse=TRUE;
  while(manual_mouse) {
    /* wait until the mouse is, where it should be */
    Delay(1);
  }
  Delay(10);
}

/********************************************************
 * handle_gadget
 *
 * react on border gadget clicks
 ********************************************************/
void handle_gadget(struct Process *thread, JanusWin *jwin, UWORD gadid) {
  UWORD x,y;
  ULONG t;
  ULONG specialinfo;
  JanusGadget *jgad=NULL;

  JWLOG("aros_win_thread[%lx]: jwin %lx, gadid %d\n", thread, jwin, gadid);

  switch (gadid) {
    case GAD_DOWNARROW:
    case GAD_UPARROW:
    case GAD_LEFTARROW:
    case GAD_RIGHTARROW:
      JWLOG("aros_win_thread[%lx]: GAD_DOWNARROW etc\n", thread);
      if(!jwin->jgad[GAD_UPARROW] && !jwin->jgad[GAD_LEFTARROW]) {
	init_border_gadgets(thread, jwin);
      }

#if 0
      switch (gadid) {

	case GAD_DOWNARROW : jgad=jwin->arrow_down;  break;
	case GAD_UPARROW   : jgad=jwin->arrow_up;    break;
	case GAD_LEFTARROW : jgad=jwin->arrow_left;  break;
	case GAD_RIGHTARROW: jgad=jwin->arrow_right; break;
      }
#endif
      jgad=jwin->jgad[gadid];
      if(!jgad) {
	JWLOG("aros_win_thread[%lx]: jgad not matched??\n", thread);
	/* should not happen */
	break;
      }

      /* right border end of aos3 window */
      x=jwin->aroswin->LeftEdge + jwin->aroswin->Width - jwin->aroswin->BorderRight;
      /* those values are negative */
      manual_mouse_x=x + get_word(jgad->aos3gadget + 4) + (get_word(jgad->aos3gadget +  8)/2);

      y=jwin->aroswin->TopEdge + jwin->aroswin->Height - jwin->aroswin->BorderBottom;
      manual_mouse_y=y + get_word(jgad->aos3gadget + 6) + (get_word(jgad->aos3gadget + 10)/2);

      JWLOG("aros_win_thread[%lx]: hit gadget %lx at %d x %d..\n", thread, jgad->aos3gadget, manual_mouse_x, manual_mouse_y);

      mice[0].enabled=FALSE; /* disable mouse emulation */
      manual_mouse=TRUE;
      while(manual_mouse) {
	/* wait until the mouse is, where it should be */
	Delay(1);
      }
      Delay(10);

      my_setmousebuttonstate(0, 0, 1); /* click */


    break;

    case GAD_HORIZSCROLL:
      JWLOG("GAD_HORIZSCROLL!\n");
 
      move_horiz_prop_gadget(thread, jwin); 
      my_setmousebuttonstate(0, 0, 1); /* click */
      horiz_prop_active=TRUE;
      JWLOG("horiz_prop_active=TRUE\n");

    break;

    case GAD_VERTSCROLL:
      JWLOG("GAD_VERTSCROLL!\n");
 
      move_vert_prop_gadget(thread, jwin); 
      my_setmousebuttonstate(0, 0, 1); /* click */
      vert_prop_active=TRUE;
      JWLOG("vert_prop_active=TRUE\n");

    break;


    default:
      JWLOG("aros_win_thread[%lx]: WARNING: gadid %d is not handled (yet?)\n", thread, gadid);
    break;
  }

}


void dump_prop_gadget(struct Process *thread, ULONG gadget) {
  UWORD  gadget_type;
  ULONG  specialinfo;
  UWORD  flags;

  gadget_type =get_word(gadget + 16); 

  if((gadget_type & GTYP_GTYPEMASK) != GTYP_PROPGADGET) {
    JWLOG("aros_win_thread[%lx]: gadget %lx is not a  GTYP_PROPGADGET\n", thread, gadget);
  }

  specialinfo=get_long(gadget + 34);
  JWLOG("aros_win_thread[%lx]: gadget %lx SpecialInfo: %lx\n", thread, gadget, specialinfo);

  if(!specialinfo) {
    return;
  }

  flags=get_word(specialinfo);
  JWLOG("aros_win_thread[%lx]: flags %lx\n", thread, flags);
  if(flags & FREEHORIZ) {
    JWLOG("aros_win_thread[%lx]: flag FREEHORIZ\n", thread);
    JWLOG("aros_win_thread[%lx]: HorizPot %d\n", thread, get_word(specialinfo+2));
    JWLOG("aros_win_thread[%lx]: HorizBody %d\n", thread, get_word(specialinfo+6));
  }
  if(flags & FREEVERT) {
    JWLOG("aros_win_thread[%lx]: flag FREEVERT\n", thread);
    JWLOG("aros_win_thread[%lx]: VertPot %d\n", thread, get_word(specialinfo+4));
    JWLOG("aros_win_thread[%lx]: VertBody %d\n", thread, get_word(specialinfo+8));
  }
}

/***********************************************************
 * make_gadgets(jwin)
 *
 * create border gadgets
 ***********************************************************/
struct Gadget *make_gadgets(struct Process *thread, JanusWin* jwin) {
    IPTR imagew[NUM_IMAGES], imageh[NUM_IMAGES];
    WORD v_offset, h_offset, btop, i;
    struct Gadget   *vertgadget =NULL;
    struct Gadget   *horizgadget=NULL;
    WORD img2which[] = {
   	UPIMAGE, 
   	DOWNIMAGE, 
   	LEFTIMAGE, 
   	RIGHTIMAGE, 
   	SIZEIMAGE
    };

   
   if(!jwin->dri) {
    jwin->dri = GetScreenDrawInfo(jwin->jscreen->arosscreen);
   }

    for(i = 0;i < NUM_IMAGES;i++) {

      jwin->img[i] = NewObject(0, SYSICLASS, SYSIA_DrawInfo, (Tag) jwin->dri, 
       					SYSIA_Which, (Tag) img2which[i], 
					TAG_DONE);

      if (!jwin->img[i]) {
	JWLOG("aros_win_thread[%lx]: could not create image %d\n", thread, i);
	return NULL;
      }

      GetAttr(IA_Width,  (Object *) jwin->img[i], &imagew[i]);
      GetAttr(IA_Height, (Object *) jwin->img[i], &imageh[i]);
    }

    btop = jwin->jscreen->arosscreen->WBorTop + jwin->dri->dri_Font->tf_YSize + 1;

    JWLOG("aros_win_thread[%lx]: btop=%d\n", thread, btop);
    JWLOG("aros_win_thread[%lx]: imagew[IMG_UPARROW]  =%d\n", thread, imagew[IMG_UPARROW]);
    JWLOG("aros_win_thread[%lx]: imageh[IMG_DOWNARROW]=%d\n", thread, imageh[IMG_DOWNARROW]);

    v_offset = imagew[IMG_DOWNARROW] / 4;
    h_offset = imageh[IMG_LEFTARROW] / 4;

    if(jwin->jgad[GAD_UPARROW]) {
      vertgadget = 
      jwin->gad[GAD_UPARROW] = NewObject(0, BUTTONGCLASS, GA_Image, (Tag) jwin->img[IMG_UPARROW], 
					  GA_RelRight, -imagew[IMG_UPARROW] + 1, 
					  GA_RelBottom, -imageh[IMG_DOWNARROW] - imageh[IMG_UPARROW] - imageh[IMG_SIZE] + 1, 
					  GA_ID, GAD_UPARROW, 
					  GA_RightBorder, TRUE, 
					  GA_Immediate, TRUE,
					  GA_RelVerify, TRUE, 
					  GA_GZZGadget, TRUE,
					  TAG_DONE);

      jwin->gad[GAD_DOWNARROW] = NewObject(0, BUTTONGCLASS, GA_Image, (Tag) jwin->img[IMG_DOWNARROW], 
					  GA_RelRight, -imagew[IMG_UPARROW] + 1, 
					  GA_RelBottom, -imageh[IMG_UPARROW] - imageh[IMG_SIZE] + 1, 
					  GA_ID, GAD_DOWNARROW, 
					  GA_RightBorder, TRUE, 
					  GA_Previous, (Tag) jwin->gad[GAD_UPARROW], 
					  GA_Immediate, TRUE,
					  GA_RelVerify    , TRUE, 
					  GA_GZZGadget, TRUE,
					  TAG_DONE);

      jwin->gad[GAD_VERTSCROLL] = NewObject(0, PROPGCLASS, GA_Top, btop + 1, 
					  GA_RelRight, -imagew[IMG_DOWNARROW] + v_offset + 1, 
					  GA_Width, imagew[IMG_DOWNARROW] - v_offset * 2,
					  GA_RelHeight, -imageh[IMG_DOWNARROW] - imageh[IMG_UPARROW] - imageh[IMG_SIZE] - btop -2, 
					  GA_ID, GAD_VERTSCROLL, 
					  GA_Previous, (Tag) jwin->gad[GAD_DOWNARROW], 
					  GA_RightBorder, TRUE, 
					  GA_RelVerify, TRUE, 
					  GA_Immediate, TRUE, 
					  PGA_NewLook, TRUE, 
					  PGA_Borderless, TRUE, 
#if 0
					  PGA_Total, , 
					  PGA_Visible, 100, 
#endif
					  PGA_VertPot,    MAXPOT,
					  PGA_VertBody,   MAXBODY,
					  PGA_Freedom, FREEVERT, 
					  GA_GZZGadget, TRUE,
					  TAG_DONE);
      for(i = 0;i < 3;i++) {
	  JWLOG("aros_win_thread[%lx]: gad[%d]=%lx\n", thread, i, jwin->gad[i]);
  	  if (!jwin->gad[i]) {
  	    JWLOG("aros_win_thread[%lx]: ERROR: gad[%d]==NULL\n", thread, i);
  	    return NULL;
  	  }
      }
    }

    if(jwin->jgad[GAD_LEFTARROW]) {
      horizgadget=
      jwin->gad[GAD_RIGHTARROW] = NewObject(0, BUTTONGCLASS, GA_Image, (Tag) jwin->img[IMG_RIGHTARROW], 
					  GA_RelRight, -imagew[IMG_SIZE] - imagew[IMG_RIGHTARROW] + 1, 
					  GA_RelBottom, -imageh[IMG_RIGHTARROW] + 1, 
					  GA_ID, GAD_RIGHTARROW, 
					  GA_BottomBorder, TRUE, 
					  jwin->jgad[GAD_UPARROW] ? GA_Previous : TAG_IGNORE, (Tag) jwin->gad[GAD_VERTSCROLL], 
					  GA_Immediate, TRUE, 
					  GA_RelVerify, TRUE,
					  GA_GZZGadget, TRUE,
					  TAG_DONE);

       jwin->gad[GAD_LEFTARROW] = NewObject(0, BUTTONGCLASS, GA_Image, (Tag) jwin->img[IMG_LEFTARROW], 
					  GA_RelRight, -imagew[IMG_SIZE] - imagew[IMG_RIGHTARROW] - imagew[IMG_LEFTARROW] + 1, 
					  GA_RelBottom, -imageh[IMG_RIGHTARROW] + 1, 
					  GA_ID, GAD_LEFTARROW, 
					  GA_BottomBorder, TRUE, 
					  GA_Previous, (Tag) jwin->gad[GAD_RIGHTARROW], 
					  GA_Immediate, TRUE, 
					  GA_RelVerify, TRUE ,
					  GA_GZZGadget, TRUE,
					  TAG_DONE);

       jwin->gad[GAD_HORIZSCROLL] = NewObject(0, PROPGCLASS, GA_Left, jwin->jscreen->arosscreen->WBorLeft, 
					  GA_RelBottom, -imageh[IMG_LEFTARROW] + h_offset + 1, 
					  GA_RelWidth, -imagew[IMG_LEFTARROW] - imagew[IMG_RIGHTARROW] - imagew[IMG_SIZE] - jwin->jscreen->arosscreen->WBorRight - 2, 
					  GA_Height, imageh[IMG_LEFTARROW] - (h_offset * 2), 
					  GA_ID, GAD_HORIZSCROLL, 
					  GA_Previous, (Tag) jwin->gad[GAD_LEFTARROW], 
					  GA_BottomBorder, TRUE, 
					  GA_RelVerify, TRUE, 
					  GA_Immediate, TRUE, 
					  PGA_NewLook, TRUE, 
					  PGA_Borderless, TRUE, 
					  PGA_Total, 80, 
					  PGA_Visible, 80, 
					  PGA_Freedom, FREEHORIZ, 
					  GA_GZZGadget, TRUE,
					  TAG_DONE);
      for(i = 3;i < NUM_GADGETS;i++) {
  	  if (!jwin->gad[i]) {
  	    JWLOG("aros_win_thread[%lx]: ERROR: gad[%d]==NULL\n", thread, i);
  	    return NULL;
  	  }
      }

    }

    if(jwin->jgad[GAD_UPARROW]) {
      return vertgadget;
    }

    return horizgadget;
}

/***********************************************************
 * remove_gadgets(jwin)
 *
 * remove all border gadgets
 ***********************************************************/
static void de_init_border_gadgets(struct Process *thread, JanusWin *jwin) {
  ULONG i;

  for(i=0; i<NUM_GADGETS; i++) {
    FreePooled(jwin->mempool, jwin->jgad[i], sizeof(JanusGadget));
    jwin->jgad[i]=NULL;

  }
#if 0
  if(jwin->jgad[GAD_UPARROW]) {
    FreePooled(jwin->mempool, jgad[GAD_UPARROW]->arrow_up, sizeof(JanusGadget));
    jwin->arrow_up=NULL;
  }
  if(jwin->arrow_down) {
    FreePooled(jwin->mempool, jwin->arrow_down, sizeof(JanusGadget));
    jwin->arrow_down=NULL;
  }
  if(jwin->prop_up_down) {
    FreePooled(jwin->mempool, jwin->prop_up_down, sizeof(JanusGadget));
    jwin->prop_up_down=NULL;
  }
  if(jwin->arrow_left) {
    FreePooled(jwin->mempool, jwin->arrow_left, sizeof(JanusGadget));
    jwin->arrow_left=NULL;
  }
  if(jwin->arrow_right) {
    FreePooled(jwin->mempool, jwin->arrow_right, sizeof(JanusGadget));
    jwin->arrow_right=NULL;
  }
  if(jwin->prop_left_right) {
    FreePooled(jwin->mempool, jwin->prop_left_right, sizeof(JanusGadget));
    jwin->prop_left_right=NULL;
  }
#endif
}

void remove_gadgets(struct Process *thread, JanusWin* jwin) {
  ULONG i;

  if(!jwin->aroswin || !jwin->firstgadget) {
    /* nothing to do */
    return;
  }

  JWLOG("RemoveGList ..\n");
  RemoveGList( jwin->aroswin, jwin->firstgadget, -1 );

  for(i=0; i<NUM_GADGETS; i++) {
    JWLOG("DisposeObject(%lx)\n", jwin->gad[i]);
    DisposeObject(jwin->gad[i]);
    jwin->gad[i]=NULL;
  }

  JWLOG("de_init_border_gadgets ..\n");
  de_init_border_gadgets(thread, jwin);
}

/* 
 * AROS creates PROPGADGETS as GTYP_CUSTOMGADGET, *not*
 * as GTYP_PROPGADGET. 
 * See AROS/rom/intuition/propgclass.c 
 *
 * Only when needed (?) propgclass changes the type to 
 * GTYP_PROPGADGET and switches back as soon as possible
 * to GTYP_CUSTOMGADGET.
 *
 * As I don't understand that magic, I don't want to change 
 * that in AROS. We just do the same. If we don't do it,
 * NewModifyProp won't work, as it checks for GTYP_PROPGADGET
 * and does nothing for a GTYP_CUSTOMGADGET.
 *
 */
UWORD SetGadgetType(struct Gadget *gad, UWORD type) {
  UWORD oldtype;

  oldtype=gad->GadgetType;

  gad->GadgetType &= ~GTYP_GTYPEMASK; 
  gad->GadgetType |= type;

  return oldtype;
}

