/************************************************************************ 
 *
 * splash.c
 *
 * GTK splash screen
 *
 * Copyright 2012 Oliver Brunner - aros<at>oliver-brunner.de
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gtk/gtkwindow.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "threaddep/thread.h"
#include "debug.h"

#include "splash.h"

#define GUI_DEBUG 1
#ifdef  GUI_DEBUG
#define DEBUG_LOG(...) do { kprintf("%s:%d %s(): ",__FILE__,__LINE__,__func__);kprintf(__VA_ARGS__); } while(0)
#else
#define DEBUG_LOG(...) do ; while(0)
#endif

static jSplashClass *parent_class= NULL;

enum {
  TEXT_CHANGE_SIGNAL,
  TIME_CHANGE_SIGNAL,
  LAST_SIGNAL
};

static guint jsplash_signals[LAST_SIGNAL];

/***********************************************
 * jsplash_class_init
 ***********************************************/
static void jsplash_class_init (jSplashClass *class) {
  GtkWidgetClass *widget_class;
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;
  parent_class = g_type_class_peek_parent (class);

  gtkutil_add_signals_to_class (object_class,
				0,
				jsplash_signals,
				"text-changed",
				"time-changed",
				(void*)0);
}

/***********************************************
 * jsplash_init
 *
 * empty, as we do it in jsplash_new.
 ***********************************************/
static void jsplash_init (jSplash *splash) {

  DEBUG_LOG("jsplash_init\n");

}

/***********************************************
 * jsplash_get_type
 *
 * return type, if none exists.
 * create new type otherwise.
 ***********************************************/
GType jsplash_get_type (void) {

  static GType jsplash_type = 0;

  if (!jsplash_type) {
      static const GTypeInfo jsplash_info = {
        sizeof (jSplashClass),
        NULL,		/* base_init */
        NULL,		/* base_finalize */
        (GClassInitFunc) jsplash_class_init,
        NULL,		/* class_finalize */
        NULL,		/* class_data */
        sizeof (jSplash),
        0,		/* n_preallocs */
        (GInstanceInitFunc) jsplash_init,
	NULL,           /* value_table */
      };
      jsplash_type = g_type_register_static (GTK_TYPE_WINDOW, "jSplash", &jsplash_info, 0);
      parent_class = g_type_class_ref (GTK_TYPE_WINDOW);
    }

  return jsplash_type;
}

/***********************************************
 * jsplash_new
 ***********************************************/
GtkWidget* jsplash_new (void) {
  GtkWidget *jsplash;
  GtkWidget *button;

  DEBUG_LOG("entered\n");

  jsplash=g_object_new (TYPE_JSPLASH, NULL);

  button = gtk_button_new_with_label ("Hello World");

  gtk_container_add (GTK_CONTAINER (jsplash), button);
  gtk_widget_show (button);

#if 0
  gtk_signal_connect (GTK_OBJECT (jsplash), "read-prefs",
			  GTK_SIGNAL_FUNC (read_prefs),
			  NULL);
  gtk_signal_connect (GTK_OBJECT (jsplash), "lock-it",
			  G_CALLBACK (lock_it),
			  NULL);
  gtk_signal_connect (GTK_OBJECT (jsplash), "unlock-it",
			  GTK_SIGNAL_FUNC (unlock_it),
			  NULL);
/*
  gtk_signal_connect (GTK_OBJECT (jdisp), "destroy",
			  GTK_SIGNAL_FUNC (jsplash_destroy),
			  NULL);
*/
#endif

  return jsplash;
}

/* is not called, even if enabled ? */
/*
static void jsplash_destroy(GtkObject *object) {

  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_JINTEGRATION (object));



  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}
*/


/***********************************************
 * change_lock (jSplash *j, gboolean status)
 *
 * updates the labels to reflect the current
 * states of the daemons.
 *
 * status is not used any more. 
 ***********************************************/
#if 0
static void change_lock (jSplash *j, gboolean status) {
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
#endif

/************************** callbacks *************************/

static void text_changed(GtkWidget *me, jSplash *j) {

  DEBUG_LOG("TODO!\n");
#if 0
  GtkList *list   = GTK_LIST (GTK_COMBO (j->combo_clip)->list);
  GList   *choice = list->selection;
  gint     sel;

  if (!choice) {
    return;
  }

  sel=gtk_list_child_position (list, choice->data);

  j->clipboard=(gboolean) sel;

  g_signal_emit_by_name(j,"clipboard-changed",j);
#endif

}

static void time_changed(GtkWidget *win, jSplash *j) {

  DEBUG_LOG("TODO!\n");
#if 0
  GtkList *list   = GTK_LIST (GTK_COMBO (j->combo_mouse)->list);
  GList   *choice = list->selection;
  gint     sel;

  if (!choice) {
    return;
  }

  sel=gtk_list_child_position (list, choice->data);

  j->mouse=(gboolean) sel;

  g_signal_emit_by_name(j,"mouse-changed",j);
#endif
}

static void hide(GtkWidget *win, jSplash *j) {
#if 0

  GtkList *list   = GTK_LIST (GTK_COMBO (j->combo_coherence)->list);
  GList   *choice = list->selection;
  gint     sel;

  if (!choice) {
    return;
  }

  sel=gtk_list_child_position (list, choice->data);

  j->coherence=(gboolean) sel;

  g_signal_emit_by_name(j,"coherent-changed",j);
#endif
}

static void show(GtkWidget *me, jSplash *j) {

#if 0
  GtkList *list   = GTK_LIST (GTK_COMBO (j->combo_launch)->list);
  GList   *choice = list->selection;
  gint     sel;

  if (!choice) {
    return;
  }

  sel=gtk_list_child_position (list, choice->data);

  j->launch=(gboolean) sel;

  g_signal_emit_by_name(j,"launch-changed",j);
#endif

}

