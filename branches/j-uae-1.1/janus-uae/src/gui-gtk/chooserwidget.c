/*
 * chooserwidget.c
 *
 * Copyright 2003-2004 Richard Drummond
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "chooserwidget.h"
#include "util.h"


//#if defined GTKMUI
//#define MGTK_DEBUG 1
//#include "/home/oli/aros/gtk-mui/debug.h"
//#endif
//
#define DebOut(...)


static void chooserwidget_init (ChooserWidget *chooser);
static void chooserwidget_class_init (ChooserWidgetClass *class);
static guint chooser_get_choice_num (ChooserWidget *chooser);
static void on_choice_changed (GtkWidget *w, ChooserWidget *chooser);

guint chooserwidget_get_type ()
{
#if 0
    static guint chooserwidget_type = 0;

    if (!chooserwidget_type) {
	static const GtkTypeInfo chooserwidget_info = {
	    (char *) "ChooserWidget",
	    sizeof (ChooserWidget),
	    sizeof (ChooserWidgetClass),
	    (GtkClassInitFunc) chooserwidget_class_init,
	    (GtkObjectInitFunc) chooserwidget_init,
	    /*(GtkArgSetFunc)*/ NULL,
	    /*(GtkArgGetFunc)*/ NULL,
	    (GtkClassInitFunc) NULL
	};
	chooserwidget_type = gtk_type_unique (gtk_combo_get_type (), &chooserwidget_info);
    }
#endif

    static GType chooserwidget_type = 0;

    DebOut("chooserwidget_get_type()\n");

    if (!chooserwidget_type) {

      DebOut("!chooserwidget_type\n");

      static const GTypeInfo chooserwidget_info = {
	sizeof (ChooserWidgetClass),
	NULL,             /* base_init */
	NULL,             /* base_finalize */
	(GClassInitFunc) chooserwidget_class_init,
	NULL,             /* class_finalize */
	NULL,             /* class_data */
	sizeof (ChooserWidget),
	0,                /* n_preallocs */
	(GInstanceInitFunc) chooserwidget_init,
      };

      chooserwidget_type=g_type_register_static (GTK_TYPE_COMBO, 
                                                 "ChooserWidget",
						 &chooserwidget_info,
						 0);
      DebOut("!chooserwidget_type DONE\n");
    }

    return chooserwidget_type;
}

enum {
    CHANGE_SIGNAL,
    LAST_SIGNAL
};

static guint chooserwidget_signals[LAST_SIGNAL] = { 0 };


static void chooserwidget_class_init (ChooserWidgetClass *class)
{
    gtkutil_add_signals_to_class ((GtkObjectClass *)class,
				   GTK_STRUCT_OFFSET (ChooserWidgetClass, chooserwidget),
				   chooserwidget_signals,
				   "selection-changed",
				   0);
    class->chooserwidget = NULL;
}

static void chooserwidget_init (ChooserWidget *chooser)
{
    GtkCombo *combo = GTK_COMBO (chooser);

    gtk_combo_set_use_arrows( combo, FALSE);
    gtk_combo_set_value_in_list ( combo, TRUE, FALSE);
    gtk_entry_set_editable (GTK_ENTRY (combo->entry), FALSE);

    gtk_signal_connect (GTK_OBJECT (combo->popwin), "hide",
			GTK_SIGNAL_FUNC (on_choice_changed),
			chooser);
    return;
}

static guint chooser_get_choice_num (ChooserWidget *chooser)
{
    GtkList *list   = GTK_LIST (GTK_COMBO (chooser)->list);
    GList   *choice = list->selection;

    //printf("choice=%d\n",choice);
    //D(bug("chooser_get_choice_num: choice: %lx\n",choice));

    if (choice)
	return gtk_list_child_position (list, choice->data);
    else
	return -1;
}

static void on_choice_changed (GtkWidget *w, ChooserWidget *chooser)
{
    chooser->choice = chooser_get_choice_num (chooser);
  //D(bug("on_choice_changed: chooser->choice: %d\n",chooser->choice));
    gtk_signal_emit_by_name (GTK_OBJECT(chooser), "selection-changed");
}

GtkWidget *chooserwidget_new (void)
{
    ChooserWidget *w = CHOOSERWIDGET (gtk_type_new (chooserwidget_get_type ()));

    return GTK_WIDGET (w);
}

void chooserwidget_set_choice (ChooserWidget *chooser, guint choice_num)
{
  GtkWidget *list;
  DebOut("entered\n");

  list=GTK_COMBO(chooser)->list;

  DebOut("chooser: %lx\n",chooser);
  DebOut("chooser->list: %lx\n",list);
  DebOut("choice_num: %d\n",choice_num);

  gtk_list_select_item (GTK_LIST (list), choice_num);

  DebOut("gtk_list_select_item done\n");

  /* Need to manually emit the change signal */
  /* o1i: why!? */
  if (chooser->choice != choice_num) {
      chooser->choice = choice_num;
      gtk_signal_emit_by_name ( GTK_OBJECT(chooser), "selection-changed");
  }
}
