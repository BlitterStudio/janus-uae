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

#include "chooserwidget.h"
#include "util.h"
#include "chipsetspeedpanel.h"
#include "chipsettypepanel.h"
#include "integration.h"


/* local proto types */
static void read_prefs                (jIntegration *j);
static void unlock_it                 (jIntegration *j);
static void lock_it                   (jIntegration *j);
static void coherent_changed          (GtkWidget *me, jIntegration *j);
static void clipboard_changed         (GtkWidget *me, jIntegration *j);
static void mouse_changed             (GtkWidget *me, jIntegration *j);

static void make_integration_widgets  (GtkWidget *vbox);
static void jintegration_class_init   (jIntegrationClass *class);
static void jintegration_init         (jIntegration      *display);
/* static void jintegration_destroy(GtkObject *object); */

static jIntegrationClass *parent_class= NULL;

enum {
  COHERENT_CHANGE_SIGNAL,
  CLIPBOARD_CHANGE_SIGNAL,
  J_D_SIGNAL,
  LOCK_IT,
  UNLOCK_IT,
  READ_PREFS,
  MOUSE_CHANGE_SIGNAL,
  LAUNCH_CHANGE_SIGNAL,
  LAST_SIGNAL
};

static guint jintegration_signals[LAST_SIGNAL];

/***********************************************
 * jintegration_class_init
 ***********************************************/
static void jintegration_class_init (jIntegrationClass *class) {
  GtkWidgetClass *widget_class;
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;
  parent_class = g_type_class_peek_parent (class);

  /* object_class->destroy = jintegration_destroy; */

  gtkutil_add_signals_to_class (object_class,
				0,
				jintegration_signals,
				"coherent-changed",
				"clipboard-changed",
				"j-d",
				"lock-it",
				"unlock-it",
				"read-prefs",
				"mouse-changed",
				"launch-changed",
				(void*)0);
}

/***********************************************
 * jintegration_init
 *
 * empty, as we do it in jintegration_new.
 ***********************************************/
static void jintegration_init (jIntegration *disp) {

}

/***********************************************
 * jintegration_get_type
 *
 * return type, if none exists.
 * create new type otherwise.
 ***********************************************/
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

/***********************************************
 * jintegration_new
 ***********************************************/
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
/*
  gtk_signal_connect (GTK_OBJECT (jdisp), "destroy",
			  GTK_SIGNAL_FUNC (jintegration_destroy),
			  NULL);
*/

  return jdisp;
}

/* is not called, even if enabled ? */
/*
static void jintegration_destroy(GtkObject *object) {

  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_JINTEGRATION (object));



  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}
*/

/***********************************************
 * read_prefs
 *
 * update all widgets, so that they reflect the
 * settings in currprefs.
 *
 * it is called through the signal "read-prefs"
 ***********************************************/
static void read_prefs (jIntegration *j) {

  gtk_list_select_item (GTK_LIST (GTK_COMBO (j->combo_coherence)->list), currprefs.jcoherence);
  gtk_list_select_item (GTK_LIST (GTK_COMBO (j->combo_clip)->list),      currprefs.jclipboard);
  gtk_list_select_item (GTK_LIST (GTK_COMBO (j->combo_mouse)->list),     currprefs.jmouse);
  gtk_list_select_item (GTK_LIST (GTK_COMBO (j->combo_launch)->list),    currprefs.jlaunch);

}

/* we should include j.h here .. */
extern ULONG aos3_task;
extern ULONG aos3_clip_task;
extern ULONG aos3_launch_task;

/***********************************************
 * change_lock (jIntegration *j, gboolean status)
 *
 * updates the labels to reflect the current
 * states of the daemons.
 *
 * status is not used any more. 
 ***********************************************/
static void change_lock (jIntegration *j, gboolean status) {
  int i;
  gboolean cdrunning;

  if(aos3_task) {
    gtk_label_set_text(GTK_LABEL(j->label_coherence), "AmigaOS/janusd is running");
    gtk_label_set_text(GTK_LABEL(j->label_mouse),     "AmigaOS/janusd is running");
  }
  else {
    gtk_label_set_text(GTK_LABEL(j->label_coherence), "AmigaOS/janusd is *not* running!");
    gtk_label_set_text(GTK_LABEL(j->label_mouse),     "AmigaOS/janusd is *not* running!");
  }

  if(aos3_clip_task) {
    gtk_label_set_text(GTK_LABEL(j->label_clip), "AmigaOS/clipd is running");
  }
  else {
    gtk_label_set_text(GTK_LABEL(j->label_clip), "AmigaOS/clipd is *not* running!");
  }

  if(aos3_launch_task) {
    gtk_label_set_text(GTK_LABEL(j->label_launch), "AmigaOS/launchd is running");
  }
  else {
    gtk_label_set_text(GTK_LABEL(j->label_launch), "AmigaOS/launchd is *not* running!");
  }
}

/***********************************************
 * lock_it
 *
 * used to lock all widgets, now as status
 * is ignored, just calls change_lock
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

static void clipboard_changed(GtkWidget *me, jIntegration *j) {

  GtkList *list   = GTK_LIST (GTK_COMBO (j->combo_clip)->list);
  GList   *choice = list->selection;
  gint     sel;

  if (!choice) {
    return;
  }

  sel=gtk_list_child_position (list, choice->data);

  j->clipboard=(gboolean) sel;

  g_signal_emit_by_name(j,"clipboard-changed",j);

}

static void mouse_changed(GtkWidget *win, jIntegration *j) {

  GtkList *list   = GTK_LIST (GTK_COMBO (j->combo_mouse)->list);
  GList   *choice = list->selection;
  gint     sel;

  if (!choice) {
    return;
  }

  sel=gtk_list_child_position (list, choice->data);

  j->mouse=(gboolean) sel;

  g_signal_emit_by_name(j,"mouse-changed",j);
}

static void coherence_changed(GtkWidget *win, jIntegration *j) {

  GtkList *list   = GTK_LIST (GTK_COMBO (j->combo_coherence)->list);
  GList   *choice = list->selection;
  gint     sel;

  if (!choice) {
    return;
  }

  sel=gtk_list_child_position (list, choice->data);

  j->coherence=(gboolean) sel;

  g_signal_emit_by_name(j,"coherent-changed",j);
}

static void launch_changed(GtkWidget *me, jIntegration *j) {

  GtkList *list   = GTK_LIST (GTK_COMBO (j->combo_launch)->list);
  GList   *choice = list->selection;
  gint     sel;

  if (!choice) {
    return;
  }

  sel=gtk_list_child_position (list, choice->data);

  j->launch=(gboolean) sel;

  g_signal_emit_by_name(j,"launch-changed",j);

}

/***********************************************
 * make_combo
 *
 * small helper, to create a combo_box
 * with some seletections
 ***********************************************/
static GtkWidget *make_combo(int count, ...) {
  GtkWidget *combo;
  GList     *list = NULL;
  va_list   choices;
  int       i;

  combo=gtk_combo_new();

  va_start (choices, count);
  for (i=0; i<count; i++) {
    /* I am not sure, if those strings are copied !? */
    list=g_list_append(list, (gpointer) va_arg (choices, char *));
  }
  /* nevertheless, MUI will copy the strings used. */
  gtk_combo_set_popdown_strings(GTK_COMBO (combo), list);
  g_list_free(list);

  return combo;
}

/**************************************************
 * make_line
 *
 * create a line for one daemon.
 *
 * j:       the integration tab main object
 * select:  will contain the new combo box
 * label:   will contain the new label
 * topic:   frame title
 * daemon:  text to display after the select widget
 * sigfunc: function to call, if select changes
 **************************************************/
static GtkWidget *make_line(jIntegration *j,
                            GtkWidget **select, GtkWidget **label, 
                            char *topic, char *daemon, 
			    void (*sigfunc) (void)) {

  GtkWidget *hbox, *frame;

  hbox =gtk_hbox_new (FALSE, 3);
  frame=gtk_frame_new (topic);

  *select=make_combo(2, "disabled", "enabled");
  *label=gtk_label_new(daemon);

  gtk_box_pack_start (GTK_BOX (hbox), *select, FALSE, FALSE, 8);
  gtk_box_pack_start (GTK_BOX (hbox), *label,  FALSE, FALSE, 8);

  gtk_container_add (GTK_CONTAINER (frame), hbox);

  gtk_signal_connect (GTK_OBJECT (GTK_COMBO(*select)->popwin), "hide", (GtkSignalFunc) sigfunc, j);

  return frame;
}

/**************************************************
 * make_integration_widgets
 *
 * fill the new widget with children.
 *
 * It would be nice of course, to make a new
 * widget class for the line objects, but it
 * would not bring much besides more OOP.
 *
 * The implementaion is hidden from the main
 * GUI anyways, as it only talks through signals
 * to the integration widget.
 **************************************************/
static void make_integration_widgets (GtkWidget *vbox) {
  jIntegration  *j=JINTEGRATION(vbox);
  GtkWidget *box;
  GtkWidget *frame_coherent;
  GtkWidget *frame_clipboard;
  GtkWidget *frame_mouse;

  if(!GTK_IS_HBOX(vbox)) {
    /* this error should not occure */
    printf("ERROR: integration.c: !GTK_IS_HBOX(%lx)\n", vbox);
    return;
  }

  /* empty box on top */
  gtk_box_pack_start (GTK_BOX (vbox), gtk_vbox_new (FALSE, 0), TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (vbox), 
		     make_line(j, 
		               &(j->combo_coherence), &(j->label_coherence), 
			       "Coherency", "AmigaOS/janusd is *not* running!",
		               (void *) coherence_changed)
		    );
  gtk_container_add (GTK_CONTAINER (vbox), 
		     make_line(j, 
		               &(j->combo_mouse), &(j->label_mouse), 
			       "Mouse Sync", "AmigaOS/janusd is *not* running!",
		               (void *) mouse_changed)
		    );
  gtk_container_add (GTK_CONTAINER (vbox), 
		     make_line(j, 
		               &(j->combo_clip), &(j->label_clip), 
			       "Clip Sync", "AmigaOS/clipd is *not* running!",
		               (void *) clipboard_changed)
		    );
  gtk_container_add (GTK_CONTAINER (vbox), 
		     make_line(j, 
		               &(j->combo_launch), &(j->label_launch), 
			       "Wanderer Integration", "AmigaOS/launchd is *not* running!",
		               (void *) launch_changed) 
		    );

  /* empty box on bottom */
  gtk_box_pack_start (GTK_BOX (vbox), gtk_vbox_new (FALSE, 0), TRUE, TRUE, 0);
}
