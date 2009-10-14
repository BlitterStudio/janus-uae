/************************************************************************ 
 *
 * display.c
 *
 * GTK notebook page for display settings
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

#ifdef __AROS__
#include <proto/intuition.h>
#include <proto/cybergraphics.h>
#include <libraries/cybergraphics.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gtk/gtk.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "gui.h"
#include "newcpu.h"
#include "filesys.h"
#include "threaddep/thread.h"
#include "audio.h"
#include "savestate.h"
#include "debug.h"
#include "inputdevice.h"
#include "xwin.h"
#include "picasso96.h"
#include "version.h"

#include "displaypanel.h"
#include "chooserwidget.h"
#include "util.h"
#include "chipsetspeedpanel.h"
#include "chipsettypepanel.h"
#include "display.h"

/* does not work, so don't enable it */
#define ENABLE_CENTERING 0
#define ENABLE_FULL_RTG_SETTINGS 0

enum {
  CHIPSET_CHANGE_SIGNAL,
  LINEMODE_CHANGE,
#if 0
  FRAMERATE_CHANGE,
  COLLISIONS_CHANGE,
  BLITS_CHANGE,
#endif
  SETTING_CHANGE,
  CENTERING_CHANGE,
  SCREEN_CHANGE,
  WIN_CHANGE,
  READ_PREFS,
  LOCK_IT,
  UNLOCK_IT,
  LAST_SIGNAL
};

static guint jdisplay_signals[LAST_SIGNAL];

GtkWidget *make_radio_group_box_param (const char *title, const char **labels,
					GtkWidget **saveptr, int horiz,
					void (*sigfunc) (void),
					GtkWidget *parameter);

int find_current_toggle  (GtkWidget **widgets, int count);
//static void jdisplay_destroy(GtkObject *object);

static void read_prefs       (jDisplay *j);
static void unlock_it        (jDisplay *j);
static void lock_it          (jDisplay *j);
static void chipset_changed  (GtkWidget *me, jDisplay *j);
static void palntsc_changed  (GtkWidget *me, jDisplay *j);
static void linemode_changed (GtkWidget *me, jDisplay *j);
static void centering_changed(GtkWidget *me, jDisplay *j);
static void settings_changed (GtkWidget *me, jDisplay *j);


GtkWidget *chipsetspeed_panel;
//GtkWidget *chipsettype_panel;
GtkWidget *chipsize_widget[5];
GtkWidget *fastsize_widget[5];

/* local proto types */
static GList       *create_resolutions          (void);
static void         make_display_widgets        (GtkWidget     *vbox);
static void         jdisplay_class_init         (jDisplayClass *class);
static void         jdisplay_init               (jDisplay      *display);

static const char *linemode_labels[] = {
	"Normal", "Doubled", "Scanline", NULL
};
static const char *chipset_labels[] = {
	"OCS", "ECS Agnus", "ECS Denise", "Full ECS", "AGA", NULL
};
static const char *pal_ntsc_labels[] = {
	"PAL", "NTSC", NULL
};
static const char *centering_labels[] = {
	"None", "Horizontal", "Vertical", "Both", NULL
};

static jDisplayClass *parent_class= NULL;

static void jdisplay_class_init (jDisplayClass *class) {
  GtkWidgetClass *widget_class;
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;
  parent_class = g_type_class_peek_parent (class);

  //object_class->destroy = jdisplay_destroy; 

  gtkutil_add_signals_to_class (object_class,
				0,
				jdisplay_signals,
				"chipset-changed",
				"linemode-changed",
#if 0
				/* those are handled by the child .. */
				"framerate-changed",
				"sprite-collisions-changed",
				"immediate-blits-changed",
#endif
				"settings-changed",
				"centering-changed",
				"screen-changed",
				"window-changed",
				"lock-it",
				"unlock-it",
				"read-prefs",
				(void*)0);


}

static void jdisplay_init (jDisplay *disp) {

  //printf("jdisplay_init(%lx)\n",disp);
}

GType jdisplay_get_type (void) {

  static GType jdisplay_type = 0;

  if (!jdisplay_type) {
      static const GTypeInfo jdisplay_info = {
        sizeof (jDisplayClass),
        NULL,		/* base_init */
        NULL,		/* base_finalize */
        (GClassInitFunc) jdisplay_class_init,
        NULL,		/* class_finalize */
        NULL,		/* class_data */
        sizeof (jDisplay),
        0,		/* n_preallocs */
        (GInstanceInitFunc) jdisplay_init,
	NULL,           /* value_table */
      };
      jdisplay_type = g_type_register_static (GTK_TYPE_HBOX, "jDisplay", &jdisplay_info, 0);
    }

  return jdisplay_type;
}

GtkWidget* jdisplay_new (void) {
  GtkWidget *jdisp;

  jdisp=g_object_new (TYPE_JDISPLAY, NULL);

  make_display_widgets(jdisp);

  gtk_signal_connect (GTK_OBJECT (jdisp), "read-prefs",
			  GTK_SIGNAL_FUNC (read_prefs),
			  NULL);
  gtk_signal_connect (GTK_OBJECT (jdisp), "lock-it",
			  G_CALLBACK (lock_it),
			  NULL);
  gtk_signal_connect (GTK_OBJECT (jdisp), "unlock-it",
			  GTK_SIGNAL_FUNC (unlock_it),
			  NULL);

#if 0
  gtk_signal_connect (GTK_OBJECT (jdisp), "destroy",
			  GTK_SIGNAL_FUNC (jdisplay_destroy),
			  NULL);
#endif


  return jdisp;
}

/* is not called, even if enabled ? */
#if 0
static void jdisplay_destroy(GtkObject *object) {

  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_JDISPLAY (object));



  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}
#endif

/***********************************************
 * read_prefs
 *
 * update all widgets, so that they reflect the
 * settings in currprefs.
 *
 * it is called through the signal "read-prefs"
 ***********************************************/
static void read_prefs (jDisplay *j) {
  gchar  *t;
  GList *r;
  ULONG   i;

  /* OCS, ECS, AGA */
  /* even if ocs is choosen in uaerc, it might end up as ECS Agnus, if
   * too much chip ram for ocs is selected.
   */

  /* These are the masks that are ORed together in the chipset_mask option.
   * If CSMASK_AGA is set, the ECS bits are guaranteed to be set as well.  
   * #define CSMASK_ECS_AGNUS 1
   * #define CSMASK_ECS_DENISE 2
   * #define CSMASK_AGA 4
   */

  if(currprefs.chipset_mask==0) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(j->chipset_widget[0]),TRUE);
  }
  else if(currprefs.chipset_mask == CSMASK_ECS_AGNUS) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(j->chipset_widget[1]),TRUE);
  }
  else if(currprefs.chipset_mask == CSMASK_ECS_DENISE) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(j->chipset_widget[2]),TRUE);
  }
  else if(currprefs.chipset_mask == (CSMASK_ECS_DENISE | CSMASK_ECS_AGNUS)) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(j->chipset_widget[3]),TRUE);
  }
  else if(currprefs.chipset_mask == (CSMASK_AGA | 
                                     CSMASK_ECS_DENISE | 
				     CSMASK_ECS_AGNUS)) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(j->chipset_widget[4]),TRUE);
  }
  else {
    /* ERROR! */
    printf("read-prefs: unknown chipset %d !?\n",currprefs.chipset_mask);
  }
  j->chipset_mask=currprefs.chipset_mask;

  /* PAL/NTSC */
  if(currprefs.ntscmode ==TRUE) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(j->pal_ntsc_widget[1]),TRUE);
  }
  else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(j->pal_ntsc_widget[0]),TRUE);
  }
  j->ntscmode=currprefs.ntscmode;

  /* centering */
  if(!currprefs.gfx_xcenter && !currprefs.gfx_ycenter) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(j->centering_widget[0]),
                                 TRUE);
  }
  else if(currprefs.gfx_xcenter && !currprefs.gfx_ycenter) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(j->centering_widget[1]),
                                 TRUE);
  }
  else if(!currprefs.gfx_xcenter && currprefs.gfx_ycenter) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(j->centering_widget[2]),
                                 TRUE);
  }
  else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(j->centering_widget[3]),
                                 TRUE);
  }
  j->gfx_xcenter=currprefs.gfx_xcenter;
  j->gfx_ycenter=currprefs.gfx_ycenter;

  /* line mode */
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
                                  j->linemode_widget[currprefs.gfx_linedbl]),
			       TRUE);
  j->gfx_linedbl=currprefs.gfx_linedbl;

  /* settings */
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(j->settings_correctaspect), 
                               currprefs.gfx_correct_aspect);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(j->settings_lores), 
                               currprefs.gfx_lores);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(j->settings_fullscreen), 
                               currprefs.gfx_afullscreen);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(j->settings_fullscreen_rtg), 
                               currprefs.gfx_pfullscreen);
  j->gfx_correct_aspect  =currprefs.gfx_correct_aspect;
  j->gfx_lores           =currprefs.gfx_lores;
  j->gfx_fullscreen_p96  =currprefs.gfx_pfullscreen;
  j->gfx_fullscreen_amiga=currprefs.gfx_afullscreen;

  /* sprite collisions etc. */
  chipsetspeedpanel_set_framerate      (CHIPSETSPEEDPANEL (chipsetspeed_panel),
                                        currprefs.gfx_framerate);
  chipsetspeedpanel_set_collision_level(CHIPSETSPEEDPANEL (chipsetspeed_panel),
                                        currprefs.collision_level);
  chipsetspeedpanel_set_immediate_blits(CHIPSETSPEEDPANEL (chipsetspeed_panel), 
                                        currprefs.immediate_blits);

  /* window width/height */
  t=g_strdup_printf("%d",currprefs.gfx_width_win);
  gtk_entry_set_text(GTK_ENTRY(j->screen_width), t);
  g_free(t);
  t=g_strdup_printf("%d",currprefs.gfx_height_win);
  gtk_entry_set_text(GTK_ENTRY(j->screen_height), t);
  g_free(t);

  /* screen resolution
   *  this is just a string compare, not nice, I know ;)
   */
  t=g_strdup_printf("%d x %d",currprefs.gfx_width_fs,currprefs.gfx_height_fs);
  r=j->res_items;
  i=0;
  while(r && strcmp(t, r->data)) {
    i++;
    r=g_list_next(r);
  }
  //printf("read_prefs: j->chipset_mask %d\n",j->chipset_mask);
  g_free(t);
  gtk_list_select_item (GTK_LIST (GTK_COMBO (j->screen_resolutions)->list), i);

}

static void change_lock (jDisplay *j, gboolean status) {
  int i;

  gtk_widget_set_sensitive(j->screen_resolutions, status);
  gtk_widget_set_sensitive(j->screen_width, status);
  gtk_widget_set_sensitive(j->screen_height, status);
  gtk_widget_set_sensitive(j->settings_correctaspect, status);
  gtk_widget_set_sensitive(j->settings_lores, status);
  gtk_widget_set_sensitive(j->settings_fullscreen, status);
#if ENABLE_FULL_RTG_SETTINGS
  gtk_widget_set_sensitive(j->settings_fullscreen_rtg, status);
#endif
  gtk_widget_set_sensitive(j->emuspeed, status);

  i=0;
  /* this can crash !? */
  while(j->linemode_widget[i]) {
    kprintf("display.c: j->linemode_widget[%d]=%lx\n",i,j->linemode_widget[i]);
    gtk_widget_set_sensitive(j->linemode_widget[i++], status);
  }
#if ENABLE_CENTERING
  i=0;
  while(j->centering_widget[i]) {
    gtk_widget_set_sensitive(j->centering_widget[i++], status);
  }
#endif
  i=0;
  while(j->centering_widget[i]) {
    gtk_widget_set_sensitive(j->centering_widget[i++], status);
  }
  i=0;
  while(j->chipset_widget[i]) {
    gtk_widget_set_sensitive(j->chipset_widget[i++], status);
  }
  i=0;
  while(j->pal_ntsc_widget[i]) {
    gtk_widget_set_sensitive(j->pal_ntsc_widget[i++], status);
  }
}


/***********************************************
 * lock_it
 *
 * if J-UAE is running, we should disable
 * the according options
 ***********************************************/
static void lock_it (jDisplay *j) {

  change_lock(j, FALSE);
}

/***********************************************
 * unlock_it
 ***********************************************/
static void unlock_it (jDisplay *j) {

  change_lock(j, TRUE);
}

/* create possible AROS resolutions for full screen mode */
static GList *create_resolutions(void) {

    int count = 0;
    struct Screen *screen;
    ULONG j;
    GList *result=NULL;
    gchar *item;

    static struct
    {
        int width, height;
    } modes [] =
    {
        { 320,  200 },
	{ 320,  240 },
	{ 320,  256 },
	{ 640,  200 },
	{ 640,  240 },
	{ 640,  256 },
	{ 640,  400 },
	{ 640,  480 },
	{ 640,  512 },
	{ 800,  600 },
	{ 1024, 768 }
    };

    screen=LockPubScreen(NULL);

    if(!screen) {
      write_log("ERROR: no public screen available !?\n");
      return NULL;
    }

    int bpx  = GetCyberMapAttr(screen->RastPort.BitMap, CYBRMATTR_BPPIX);

    int maxw = screen->Width;
    int maxh = screen->Height;

    for (j = 0; (j < (sizeof(modes)/sizeof(modes[0]))) && (j < MAX_PICASSO_MODES); j++)
    {
        static const int bpx2format[] = {0, RGBFF_CHUNKY, RGBFF_R5G6B5PC, 0, RGBFF_B8G8R8A8};

	if (modes[j].width > maxw || modes[j].height > maxh)
	    continue;

	/* TODO: free those items!! */
	item=g_strdup_printf("%d x %d",modes[j].width,modes[j].height);
	result=g_list_append(result,item);

#if 0
	DisplayModes[count].res.width  = modes[j].width;
        DisplayModes[count].res.height = modes[j].height;
        DisplayModes[count].depth      = bpx;
        DisplayModes[count].refresh    = 75;
#endif

        count++;
    }

    UnlockPubScreen(NULL,screen);
    return result;
}

/************************** callbacks *************************/
/* second parameter is always NULL */

/* OCS, ECS Agnus, ECS Denise, Full ECS, AGA */
static void chipset_changed(GtkWidget *me, jDisplay *j) {
  ULONG i;

  //printf("chipset_changed 1: j->chipset_mask=%d\n",j->chipset_mask);

  i=0;
  while(j->chipset_widget[i] && me!=j->chipset_widget[i]) {
    i++;
  }

  switch(i) {
    case 0:
      j->chipset_mask=0; /* OCS */
      break;
    case 1:
      j->chipset_mask=CSMASK_ECS_AGNUS; 
      break;
    case 2:
      j->chipset_mask=CSMASK_ECS_DENISE; 
      break;
    case 3:
      j->chipset_mask=CSMASK_ECS_DENISE | CSMASK_ECS_AGNUS; 
      break;
    case 4:
      j->chipset_mask=CSMASK_AGA | CSMASK_ECS_DENISE | CSMASK_ECS_AGNUS;
      break;
    default:
      /* ERROR! */
      printf("chipset_changed: unknown status %d !?\n",i);
      return;
  }
  //printf("chipset_changed 2: j->chipset_mask=%d\n",j->chipset_mask);

  g_signal_emit_by_name(j,"chipset-changed",j);
}

static void palntsc_changed(GtkWidget *me, jDisplay *j) {

  if(me == j->pal_ntsc_widget[0]) {
    j->ntscmode=0;
  }
  else if(me == j->pal_ntsc_widget[1]) {
    j->ntscmode=1;
  }
  else {
      printf("palntsc_changed: unknown status (%lx)!?\n",me);
      return;
  }

  g_signal_emit_by_name(j,"chipset-changed",j);
}

static void linemode_changed(GtkWidget *me, jDisplay *j) {
  ULONG i;

  i=0;
  while(j->linemode_widget[i] && me!=j->linemode_widget[i]) {
    i++;
  }

  switch(i) {
    case 0:
    case 1:
    case 2:
      j->gfx_linedbl=i; 
      break;
    default:
      /* ERROR! */
      printf("linemode_changed: unknown status %d !?\n",i);
      return;
  }

  g_signal_emit_by_name(j,"linemode-changed",j);
}

static void centering_changed(GtkWidget *me, jDisplay *j) {
  ULONG i;

  i=0;
  while(j->centering_widget[i] && me!=j->linemode_widget[i]) {
    i++;
  }

  switch(i) {
    case 0:
      j->gfx_xcenter=FALSE;
      j->gfx_ycenter=FALSE;
      break;
    case 1:
      j->gfx_xcenter=TRUE;
      j->gfx_ycenter=FALSE;
      break;
    case 2:
      j->gfx_xcenter=FALSE;
      j->gfx_ycenter=TRUE;
      break;
    default:
      j->gfx_xcenter=FALSE;
      j->gfx_ycenter=TRUE;
  }
  g_signal_emit_by_name(j,"centering-changed",j);
}

static void settings_changed(GtkWidget *me, jDisplay *j) {

  if     (me == j->settings_correctaspect) {
    j->gfx_correct_aspect=GTK_TOGGLE_BUTTON (me)->active;
  }
  else if(me == j->settings_lores) {
    j->gfx_lores=GTK_TOGGLE_BUTTON (me)->active;
  }
  else if(me == j->settings_fullscreen) {
    j->gfx_fullscreen_amiga=GTK_TOGGLE_BUTTON (me)->active;
    j->gfx_fullscreen_p96  =GTK_TOGGLE_BUTTON (me)->active;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(j->settings_fullscreen_rtg), 
                                 GTK_TOGGLE_BUTTON (me)->active);
  }
  else if(me == j->settings_fullscreen_rtg) {
    j->gfx_fullscreen_p96=GTK_TOGGLE_BUTTON (me)->active;
  }
  else {
    printf("settings_changed: unknown status !?\n");
    return;
  }
  g_signal_emit_by_name(j,"settings-changed",j);
}

static void screen_changed(GtkWidget *me, jDisplay *j) {
  gchar **wh;
  gint selected;

  /* get selection number with some simple casts..*/
  selected=gtk_list_child_position(
            GTK_LIST(GTK_COMBO (j->screen_resolutions)->list),
	    GTK_WIDGET((GList *)(GTK_LIST(GTK_COMBO (j->screen_resolutions)->list)->selection)->data));

  /* GLib rules ;) */
  wh=g_strsplit(g_list_nth_data(j->res_items,selected)," x ",2);

  j->gfx_width_fs =atoi(wh[0]);
  j->gfx_height_fs=atoi(wh[1]);

  g_strfreev(wh);

  g_signal_emit_by_name(j,"screen-changed",j);
}

static void window_changed(GtkWidget *me, jDisplay *j) {

  j->gfx_width_win =atoi(gtk_entry_get_text (GTK_ENTRY (j->screen_width)));
  j->gfx_height_win=atoi(gtk_entry_get_text (GTK_ENTRY (j->screen_height)));

  g_signal_emit_by_name(j,"window-changed",j);
}

/********************* make_display_widgets *******************/
static void make_display_widgets (GtkWidget *vbox) {
  jDisplay  *j=JDISPLAY(vbox);
  GtkWidget *box;
  GtkWidget *table;
  GtkWidget *frame_screen;
  GtkWidget *frame_screen_hbox;
  GtkWidget *frame_window;
  GtkWidget *frame_window_hbox;
  GtkWidget *frame_centering;
  GtkWidget *frame_settings;
  GtkWidget *frame_linemode;
  GtkWidget *frame_chipset;
  GtkWidget *frame_pal_ntsc;
  /* Settings */
  GtkWidget *table_settings;

  if(!GTK_IS_HBOX(vbox)) {
    printf("ERROR: display.c: !GTK_IS_HBOX(%lx)\n",vbox);
  }

  table=gtk_table_new(3,4,FALSE);

  chipsetspeed_panel=chipsetspeedpanel_new();
  /* for historical reasons (e-uae), chipsetspeed_panel needs to
   * be global.
   * better only use j->emuspeed
   */
  j->emuspeed=chipsetspeed_panel;

  gtk_table_attach_defaults(GTK_TABLE(table), chipsetspeed_panel, 0,1,3,4);

  frame_screen=   gtk_frame_new("AROS screen resolution");
  frame_window=   gtk_frame_new("AROS window size");
  frame_settings= gtk_frame_new("Settings");
  frame_centering=gtk_frame_new("Centering");
  frame_linemode= gtk_frame_new("Line Mode");

  frame_screen_hbox=gtk_hbox_new(TRUE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (frame_screen_hbox), 8);
  frame_window_hbox=gtk_hbox_new(FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (frame_window_hbox), 8);

  /* row 1 */
  gtk_table_attach_defaults(GTK_TABLE(table), frame_screen,    0, 1, 0, 1);
  gtk_table_attach_defaults(GTK_TABLE(table), frame_window,    0, 1, 1, 2);
  gtk_table_attach_defaults(GTK_TABLE(table), frame_linemode,  1, 2, 0, 2);
  gtk_table_attach_defaults(GTK_TABLE(table), frame_centering, 2, 3, 0, 2);

  gtk_container_add (GTK_CONTAINER (frame_screen), frame_screen_hbox);
  gtk_container_add (GTK_CONTAINER (frame_window), frame_window_hbox);

  /* row 2 */
  gtk_table_attach_defaults(GTK_TABLE(table), frame_settings,  0, 1, 2, 3);
  /* linemode frame done below */

  /* Amiga screen resolutions */
  j->screen_resolutions=gtk_combo_new();
  j->res_items=create_resolutions();
  gtk_combo_set_popdown_strings (GTK_COMBO (j->screen_resolutions), j->res_items);
  gtk_container_add (GTK_CONTAINER (frame_screen_hbox), 
                     gtk_label_new("Select:"));
  gtk_container_add (GTK_CONTAINER (frame_screen_hbox), j->screen_resolutions);

  j->screen_width=gtk_entry_new();
  j->screen_height=gtk_entry_new();
  gtk_container_add (GTK_CONTAINER (frame_window_hbox), 
                     gtk_label_new("Width:"));
  gtk_container_add (GTK_CONTAINER (frame_window_hbox), j->screen_width);
  gtk_container_add (GTK_CONTAINER (frame_window_hbox), 
                     gtk_label_new("Height:"));
  gtk_container_add (GTK_CONTAINER (frame_window_hbox), j->screen_height);

  gtk_signal_connect (GTK_OBJECT (GTK_COMBO(j->screen_resolutions)->popwin), 
                          "hide",
			  G_CALLBACK (screen_changed),
			  j);
  gtk_signal_connect (GTK_OBJECT (j->screen_width), 
                          "activate",
			  G_CALLBACK (window_changed),
			  j);
  gtk_signal_connect (GTK_OBJECT (j->screen_height), 
                          "activate",
			  G_CALLBACK (window_changed),
			  j);

  /* Amiga Settings */
  table_settings=gtk_table_new(2,2,TRUE);
  gtk_container_add (GTK_CONTAINER (frame_settings), table_settings);
  j->settings_correctaspect=gtk_check_button_new_with_label("Correct aspect");
  j->settings_lores=gtk_check_button_new_with_label("Lo-Res");
  j->settings_fullscreen=gtk_check_button_new_with_label("Full-Screen");
  j->settings_fullscreen_rtg=gtk_check_button_new_with_label("Full Screen RTG");

  gtk_table_attach_defaults(GTK_TABLE(table_settings), 
                            j->settings_correctaspect,    0, 1, 0, 1);
  gtk_table_attach_defaults(GTK_TABLE(table_settings), 
                            j->settings_lores,            1, 2, 0, 1);
  gtk_table_attach_defaults(GTK_TABLE(table_settings), 
                            j->settings_fullscreen,       0, 1, 1, 2);
  gtk_table_attach_defaults(GTK_TABLE(table_settings), 
                            j->settings_fullscreen_rtg,   1, 2, 1, 2);
  /* disabled TODO */
  gtk_widget_set_sensitive(j->settings_fullscreen_rtg, FALSE);

  gtk_signal_connect (GTK_OBJECT (j->settings_correctaspect), "toggled",
			  G_CALLBACK (settings_changed),
			  j);
  gtk_signal_connect (GTK_OBJECT (j->settings_lores), "toggled",
			  G_CALLBACK (settings_changed),
			  j);
  gtk_signal_connect (GTK_OBJECT (j->settings_fullscreen), "toggled",
			  G_CALLBACK (settings_changed),
			  j);
  /* disabled
  gtk_signal_connect (GTK_OBJECT (j->settings_fullscreen_rtg), "toggled",
			  G_CALLBACK (settings_changed),
			  j);
  */

  /* Line Mode */
  frame_linemode = make_radio_group_box_param ("Line Mode", 
                                        linemode_labels, 
					j->linemode_widget, 
					0, (void *)linemode_changed,
					GTK_WIDGET(j));
  gtk_table_attach_defaults(GTK_TABLE(table), frame_linemode,   1, 2, 0, 2);

  /* Centering */
  frame_centering = make_radio_group_box_param ("Centering", 
                                        centering_labels, 
					j->centering_widget, 
					0, (void *)centering_changed,
					GTK_WIDGET(j));
  gtk_widget_set_sensitive(frame_centering,FALSE);
  gtk_table_attach_defaults(GTK_TABLE(table), frame_centering,  2, 3, 0, 2);


  /* Chipset */
  frame_chipset = make_radio_group_box_param ("Chipset", 
                                          chipset_labels, 
					  j->chipset_widget, 
					  0, (void *)chipset_changed,
					  GTK_WIDGET(j));
  gtk_table_attach_defaults(GTK_TABLE(table), frame_chipset,   1, 2, 2, 4);

  /* PAL/NTSC */
  /*  GLib-GObject-CRITICAL **: 
   *  g_signal_connect_closure_by_id: assertion `signal_id > 0' failed !? */
  frame_pal_ntsc = make_radio_group_box_param ("Norm", 
                                        pal_ntsc_labels, 
					j->pal_ntsc_widget, 
					0, (void *)palntsc_changed,
					GTK_WIDGET(j));
  gtk_table_attach_defaults(GTK_TABLE(table), frame_pal_ntsc,  2, 3, 2, 4);

  gtk_widget_show_all(table);
  gtk_container_add (GTK_CONTAINER (vbox), table);
}
