/************************************************************************ 
 *
 * util.c
 *
 * Copyright 2003-2004 Richard Drummond
 * Copyright 2009-2011 Oliver Brunner - aros<at>oliver-brunner.de
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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "util.h"
#include "chooserwidget.h"

#ifdef __AROS__
#define GTKMUI
#endif


//#define GUI_DEBUG 1
#ifdef  GUI_DEBUG
#define DEBUG_LOG(...) do { kprintf("%s:%d %s(): ",__FILE__,__LINE__,__func__);kprintf(__VA_ARGS__); } while(0)
#else
#define DEBUG_LOG(...) do ; while(0)
#endif


/*
 * Some utility functions to make building a GTK+ GUI easier
 * and more compact, to hide differences between GTK1.x and GTK2.x
 * and to help maintain consistency
 */

/*
 * Create a list of signals and add them to a GTK+ class
 */
void gtkutil_add_signals_to_class (GtkObjectClass *class, guint func_offset, guint *signals, ...)
{
   va_list signames;
   const char *name;
   unsigned int count = 0;

   va_start (signames, signals);
   name = va_arg (signames, const char *);

   while (name) {
#if GTK_MAJOR_VERSION >= 2 || defined GTKMUI
	signals[count] = g_signal_new (name, G_TYPE_FROM_CLASS (class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
				func_offset, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
#else
	signals[count] = gtk_signal_new (name, GTK_RUN_FIRST, class->type, func_offset,
					gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
#endif
	count++;
	name = va_arg (signames, const char *);
   };

#if GTK_MAJOR_VERSION < 2 && !defined GTKMUI
    gtk_object_class_add_signals (class, signals, count);
#endif
}


/*
 * Make a simple chooser from a combo widget
 * <count> is the number of choices, and the choices
 * themselves are supplied as a list of strings.
 */
GtkWidget *make_chooser (int count, ...)
{
    GtkWidget *chooser;
    GList     *list = 0;
    va_list   choices;
    int       i;

    chooser = chooserwidget_new ();

    va_start (choices, count);
    for (i=0; i<count; i++)
	list = g_list_append (list, (gpointer) va_arg (choices, char *));
    gtk_combo_set_popdown_strings (GTK_COMBO (chooser), list);
    g_list_free (list);

    gtk_widget_show (chooser);
    return chooser;
}

GtkWidget *make_label (const char *string)
{
    GtkWidget *label;

    label = gtk_label_new (string);
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_widget_show (label);

    return label;
}

#if !defined GTKMUI

#if GTK_MAJOR_VERSION < 2
GtkWidget *make_labelled_button (const char *label, GtkAccelGroup *accel_group)
{
    GtkWidget *button = gtk_button_new ();
    int key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (button)->child), "_Okay");
    gtk_widget_add_accelerator (button, "clicked", accel_group,key, GDK_MOD1_MASK, (GtkAccelFlags) 0);
}
#endif

#else
GtkWidget *make_labelled_button (const char *label, GtkAccelGroup *accel_group)
{
      return gtk_button_new_with_label (label);
}
#endif


GtkWidget *make_panel (const char *name)
{
    GtkWidget *panel;

    panel = gtk_frame_new (name);
    gtk_widget_show (panel);
    gtk_container_set_border_width (GTK_CONTAINER (panel), PANEL_BORDER_WIDTH);
    gtk_frame_set_label_align (GTK_FRAME (panel), 0.01, 0.5);

    return panel;
}

GtkWidget *make_xtable( int width, int height )
{
    GtkWidget *table;

    table = gtk_table_new (height,width, FALSE);
    gtk_widget_show (table);

    gtk_container_set_border_width (GTK_CONTAINER (table), TABLE_BORDER_WIDTH);
    gtk_table_set_row_spacings (GTK_TABLE (table), TABLE_ROW_SPACING);
    gtk_table_set_col_spacings (GTK_TABLE (table), TABLE_COL_SPACING);

    return table;
}

/*
 * Add some padding to a vbox or hbox which will consume
 * space when the box is resized to larger than default size
 */
void add_box_padding (GtkWidget *box)
{
    GtkWidget *vbox;

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (box), vbox, TRUE, TRUE, 0);
}

/*
 * Add some padding to a table which will consime space when
 * when the table is resized to larger than default size
 */
void add_table_padding (GtkWidget *table, int x, int y)
{
    GtkWidget *vbox;
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_table_attach (GTK_TABLE (table), vbox, x, x+1, y, y+1,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
}

/*
 * Add a widget to a table with some sensible defaults
 *
 * <x,y> is the table position to insert the widget
 * <width> is the the number of columns the widget will take up
 * <xflags> are the attachment flags that will apply horizontally
 */
void add_to_table(GtkWidget *table, GtkWidget *widget, int x, int y, int width, int xflags)
{
  gtk_table_attach (GTK_TABLE (table), widget, x, x+width, y, y+1,
		   (GtkAttachOptions) (xflags),
		   (GtkAttachOptions) (0), 0, 0);
}


/*
 * Super-duper, handy table creation tool!
 *
 * Creates a table and add a list of widgets to it.
 */
GtkWidget *gtkutil_add_table (GtkWidget *container, ...)
{
    va_list contents;
    GtkWidget *widget;
    GtkWidget *table;
    int row, max_col;
    int col, width;
    int flags;

    table = gtk_table_new (3, 3, FALSE);
    //gtk_container_set_border_width (GTK_CONTAINER (table), TABLE_BORDER_WIDTH);
    gtk_table_set_row_spacings (GTK_TABLE (table), TABLE_ROW_SPACING);
    gtk_table_set_col_spacings (GTK_TABLE (table), TABLE_COL_SPACING);
    gtk_container_add (GTK_CONTAINER (container), table);

    va_start (contents, container);
    widget = va_arg (contents, GtkWidget *);
    row = 1;
    max_col = 1;

    while (widget != GTKUTIL_TABLE_END) {
	if (widget == GTKUTIL_ROW_END) {
	    row += 2;
	} else {
	    col   = va_arg (contents, gint);
	    if (col > max_col) max_col = col;
	    width = va_arg (contents, gint);
	    flags = va_arg (contents, gint);

	    gtk_table_attach (GTK_TABLE (table), widget, col, col+width, row, row+1,
			(GtkAttachOptions) (flags), (GtkAttachOptions) (0), 0, 0);
	}
	widget = va_arg (contents, GtkWidget *);
    }

    gtk_table_resize (GTK_TABLE (table), row, max_col + 2);
    add_table_padding (table, 0, 0);
    add_table_padding (table, max_col + 1, row - 1);

    gtk_widget_show_all (table);

    return table;
}

GtkWidget *gtkutil_make_radio_group (GSList *group, GtkWidget **buttons, ...)
{
    GtkWidget *hbox;
    va_list labels;
    const char *label;
    int i = 0;

    hbox = gtk_hbox_new (TRUE, 0);

    va_start (labels, buttons);
    label = va_arg (labels, const char *);

    while (label != NULL) {
	GtkWidget *radiobutton = gtk_radio_button_new_with_label (group, label);
	group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton));
	gtk_box_pack_start (GTK_BOX (hbox), radiobutton, FALSE, FALSE, 0);

	*buttons++ = radiobutton;

	label = va_arg (labels, const char *);
    }

    gtk_widget_show (hbox);

    return hbox;
}

/************************** taken from gtkui.c *****************************/


/*************************************************************
 * make_radio_group_param
 *
 * build radio group, maximum count entries. If count < 0,
 * build as many entries, as there are labels
 *************************************************************/

int make_radio_group (const char **labels, GtkWidget *tobox,
		      GtkWidget **saveptr, gint t1, gint t2,
		      void (*sigfunc) (void), int count, GSList *group) {

  return make_radio_group_param(labels,tobox,saveptr,t1,t2,
                                 sigfunc,count,group,NULL);
}

int make_radio_group_param (const char **label, GtkWidget *tobox,
			      GtkWidget **saveptr, gint t1, gint t2,
			      void (*sigfunc) (void), int count, GSList *group,
			      GtkWidget *parameter) {

  int        t = 0;
  GtkWidget *thing;

  if(count < 0) {
    count=10000; /* max */
  }

  while (label[t] && (t < count)) {

    DEBUG_LOG("label[%d]=%s\n", t, label[t]);

    thing = gtk_radio_button_new_with_label (group, label[t]);
    group = gtk_radio_button_group (GTK_RADIO_BUTTON (thing));

    saveptr[t] = thing;
    gtk_widget_show (thing);
    gtk_box_pack_start (GTK_BOX (tobox), thing, t1, t2, 0);
    gtk_signal_connect (GTK_OBJECT (thing), "clicked", (GtkSignalFunc) sigfunc, parameter);
    t++;
  }
  if(saveptr[t] != NULL) {
    kprintf("make_radio_group_param: old widget[%d](%lx):=%lx\n",t,&saveptr[t],saveptr[t]);
    kprintf("ERROR: saveptr[%d] is NOT NULL!\n",t);
    DEBUG_LOG("make_radio_group_param: old widget[%d](%lx):=%lx\n",t,&saveptr[t],saveptr[t]);
    DEBUG_LOG("ERROR: saveptr[%d] is NOT NULL!\n",t);
  }
  saveptr[t] = NULL; /* NULL terminate the list! */
  return t;
}


GtkWidget *make_radio_group_box_1_param (const char *title, const char **labels,
					  GtkWidget **saveptr, int horiz,
					  void (*sigfunc) (void), int elts_per_column, GtkWidget *parameter) {
  GtkWidget *frame, *newbox;
  GtkWidget *column;
  GSList *group = 0;

  frame = gtk_frame_new (title);
  column = (horiz ? gtk_vbox_new : gtk_hbox_new) (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), column);
  gtk_widget_show (column);

  while (*labels) {
    int count;
    newbox = (horiz ? gtk_hbox_new : gtk_vbox_new) (FALSE, 4);
    gtk_widget_show (newbox);
    gtk_container_set_border_width (GTK_CONTAINER (newbox), 4);
    gtk_container_add (GTK_CONTAINER (column), newbox);
    count = make_radio_group_param (labels, newbox, saveptr, horiz, !horiz, sigfunc, elts_per_column, group, parameter);
    labels += count;
    saveptr += count;
    group = gtk_radio_button_group (GTK_RADIO_BUTTON (saveptr[-1]));
  }
  return frame;
}


GtkWidget *make_radio_group_box_1 (const char *title, const char **labels,
					  GtkWidget **saveptr, int horiz,
					  void (*sigfunc) (void), int elts_per_column) {
  GtkWidget *frame, *newbox;
  GtkWidget *column;
  GSList *group = 0;

  frame = gtk_frame_new (title);
  column = (horiz ? gtk_vbox_new : gtk_hbox_new) (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), column);
  gtk_widget_show (column);

  while (*labels) {
    int count;
    newbox = (horiz ? gtk_hbox_new : gtk_vbox_new) (FALSE, 4);
    gtk_widget_show (newbox);
    gtk_container_set_border_width (GTK_CONTAINER (newbox), 4);
    gtk_container_add (GTK_CONTAINER (column), newbox);
    count = make_radio_group (labels, newbox, saveptr, horiz, !horiz, sigfunc, elts_per_column, group);
    labels += count;
    saveptr += count;
    group = gtk_radio_button_group (GTK_RADIO_BUTTON (saveptr[-1]));
  }
  return frame;
}
