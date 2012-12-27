/************************************************************************ 
 *
 * util.h
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

/*
 * Default spacing of widgets used in the
 * utility functions below
 */
#define DEFAULT_BORDER_WIDTH  8
#define PANEL_BORDER_WIDTH    DEFAULT_BORDER_WIDTH
#define TABLE_BORDER_WIDTH    DEFAULT_BORDER_WIDTH
#define TABLE_COL_SPACING     8
#define TABLE_ROW_SPACING     4


void gtkutil_add_signals_to_class (GtkObjectClass *class, guint func_offset, guint *signals, ...);

#if GTK_MAJOR_VERSION >= 2
static __inline GtkWidget *make_labelled_button (const char *label, GtkAccelGroup *accel_group)
{
    return gtk_button_new_with_mnemonic (label);
}
#else
extern GtkWidget *make_labelled_button (const char *label, GtkAccelGroup *accel_group);
#endif

extern GtkWidget *make_chooser (int count, ...);
extern GtkWidget *make_label (const char *string);
extern GtkWidget *make_panel (const char *name);
extern GtkWidget *make_xtable( int width, int height );
extern void add_box_padding (GtkWidget *box);
extern void add_table_padding (GtkWidget *table, int x, int y);
extern void add_to_table(GtkWidget *table, GtkWidget *widget, int x, int y, int width, int xflags);

#define GTKUTIL_TABLE_END	((GtkWidget *)0)
#define GTKUTIL_ROW_END		((GtkWidget *)1)

extern GtkWidget *gtkutil_add_table (GtkWidget *container, ...);
extern GtkWidget *gtkutil_make_radio_group (GSList *group, GtkWidget **buttons, ...);


int make_radio_group (const char **labels, GtkWidget *tobox,
		      GtkWidget **saveptr, gint t1, gint t2,
		      void (*sigfunc) (void), int count, GSList *group);

int make_radio_group_param (const char **label, GtkWidget *tobox,
			    GtkWidget **saveptr, gint t1, gint t2,
			    void (*sigfunc) (void), int count, GSList *group,
			    GtkWidget *parameter);


GtkWidget *make_radio_group_box_1_param (const char *title, const char **labels,
					 GtkWidget **saveptr, int horiz,
					 void (*sigfunc) (void), 
					 int elts_per_column, GtkWidget *parameter);
GtkWidget *make_radio_group_box_1 (const char *title, const char **labels,
				   GtkWidget **saveptr, int horiz,
				   void (*sigfunc) (void), int elts_per_column);


GtkWidget *make_radio_group_box_param (const char *title, const char **labels, 
                                       GtkWidget **saveptr, int horiz, 
				       void (*sigfunc) (void), GtkWidget *parameter);
GtkWidget *make_radio_group_box (const char *title, const char **labels, 
                                 GtkWidget **saveptr, int horiz, 
				 void (*sigfunc) (void));


