/************************************************************************ 
 *
 * display.c
 *
 * GTK notebook page for coherent mode settings
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
 * $Id$
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
#include "integration.h"

/* does not work, so don't enable it */
#define ENABLE_CENTERING 0
#define ENABLE_FULL_RTG_SETTINGS 0

enum {
  COHERENT_CHANGE_SIGNAL,
  CLIPBOARD_CHANGE_SIGNAL,
  J_D_SIGNAL,
  READ_PREFS,
  LOCK_IT,
  UNLOCK_IT,
  LAST_SIGNAL
};

static guint jintegration_signals[LAST_SIGNAL];

static const char *coherence_labels[]={"Classic UAE", "Full integration", NULL};
static const char *clipboard_labels[]={"Separate Clipboards", "Share Clipboard Contents", NULL};

GtkWidget *make_radio_group_box_param (const char *title, const char **labels,
					GtkWidget **saveptr, int horiz,
					void (*sigfunc) (void),
					GtkWidget *parameter);

int find_current_toggle  (GtkWidget **widgets, int count);
//static void jintegration_destroy(GtkObject *object);

/* local proto types */
static void read_prefs                (jIntegration *j);
static void unlock_it                 (jIntegration *j);
static void lock_it                   (jIntegration *j);
static void coherent_changed          (GtkWidget *me, jIntegration *j);
static void clipboard_changed         (GtkWidget *me, jIntegration *j);

static void make_integration_widgets  (GtkWidget *vbox);
static void jintegration_class_init   (jIntegrationClass *class);
static void jintegration_init         (jIntegration      *display);

static jIntegrationClass *parent_class= NULL;

static void jintegration_class_init (jIntegrationClass *class) {
  GtkWidgetClass *widget_class;
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;
  parent_class = g_type_class_peek_parent (class);

  //object_class->destroy = jintegration_destroy; 

  gtkutil_add_signals_to_class (object_class,
				0,
				jintegration_signals,
				"coherent-changed",
				"clipboard-changed",
				"j-d",
				"lock-it",
				"unlock-it",
				"read-prefs",
				(void*)0);
}

static void jintegration_init (jIntegration *disp) {

  //printf("jintegration_init(%lx)\n",disp);
}

GType jintegration_get_type (void) {

  static GType jintegration_type = 0;

  if (!jintegration_type) {
      static const GTypeInfo jintegration_info = {
        sizeof (jIntegrationClass),
        NULL,		/* base_init */
        NULL,		/* base_finalize */
        (GClassInitFunc) jintegration_class_init,
        NULL,		/* class_finalize */
        NULL,		/* class_data */
        sizeof (jIntegration),
        0,		/* n_preallocs */
        (GInstanceInitFunc) jintegration_init,
	NULL,           /* value_table */
      };
      jintegration_type = g_type_register_static (GTK_TYPE_HBOX, "jIntegration", &jintegration_info, 0);
    }

  return jintegration_type;
}

GtkWidget* jintegration_new (void) {
  GtkWidget *jdisp;

  jdisp=g_object_new (TYPE_JINTEGRATION, NULL);

  make_integration_widgets(jdisp);

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
			  GTK_SIGNAL_FUNC (jintegration_destroy),
			  NULL);
#endif


  return jdisp;
}

/* is not called, even if enabled ? */
#if 0
static void jintegration_destroy(GtkObject *object) {

  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_JINTEGRATION (object));



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
static void read_prefs (jIntegration *j) {
#if 0
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

#endif
}

static void change_lock (jIntegration *j, gboolean status) {
#if 0
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
  while(j->linemode_widget[i]) {
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
#endif
}


/***********************************************
 * lock_it
 *
 * if J-UAE is running, we should disable
 * the according options
 ***********************************************/
static void lock_it (jIntegration *j) {

  change_lock(j, FALSE);
}

/***********************************************
 * unlock_it
 ***********************************************/
static void unlock_it (jIntegration *j) {

  change_lock(j, TRUE);
}

/************************** callbacks *************************/
/* second parameter is always NULL */

static void clipboard_changed(GtkWidget *me, jIntegration *j) {
}

static void coherence_changed(GtkWidget *me, jIntegration *j) {
#if 0
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
#endif
}


/********************* make_display_widgets *******************/
static void make_integration_widgets (GtkWidget *vbox) {
  jIntegration  *j=JINTEGRATION(vbox);
  GtkWidget *box;
  GtkWidget *table;
  GtkWidget *frame_coherent;
  GtkWidget *frame_clipboard;

  if(!GTK_IS_HBOX(vbox)) {
    printf("ERROR: integration.c: !GTK_IS_HBOX(%lx)\n",vbox);
    kprintf("ERROR: integration.c: !GTK_IS_HBOX(%lx)\n",vbox);
  }

  table=gtk_table_new(1,2,FALSE);

  frame_coherent = make_radio_group_box_param ("Coherency", 
                                        coherence_labels, 
					j->coherence_widget, 
					0, (void *) coherence_changed,
					GTK_WIDGET(j));
  gtk_table_attach_defaults(GTK_TABLE(table), frame_coherent, 0,1,0,1);

  frame_clipboard = make_radio_group_box_param ("Clipboard", 
                                        clipboard_labels, 
					j->clipboard_widget, 
					0, (void *) clipboard_changed,
					GTK_WIDGET(j));
  gtk_table_attach_defaults(GTK_TABLE(table), frame_clipboard,  0, 1, 1, 2);



#if 0

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
#endif


  gtk_widget_show_all(table);
  gtk_container_add (GTK_CONTAINER (vbox), table);

}
