/************************************************************************ 
 *
 * cpuspeedpanel.c
 *
 * Copyright 2003-2005 Richard Drummond
 * Copyright 2011      Oliver Brunner - aros<at>oliver-brunner.de
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

#include "cpuspeedpanel.h"
#include "chooserwidget.h"
#include "util.h"

static void cpuspeedpanel_init (CpuSpeedPanel *cspanel);
static void cpuspeedpanel_class_init (CpuSpeedPanelClass *class);
static void update_state (CpuSpeedPanel *cspanel);
static void on_speed_changed    (GtkWidget *w, CpuSpeedPanel *cspanel);
static void on_adjust_changed   (GtkWidget *w, CpuSpeedPanel *cspanel);
static void on_idleenabled_toggled (GtkWidget *w, CpuSpeedPanel *cspanel);
static void on_idlerate_changed (GtkWidget *w, CpuSpeedPanel *cspanel);

guint cpuspeedpanel_get_type () {

  static guint cpuspeedpanel_type = 0;

  if (!cpuspeedpanel_type) {
    static const GTypeInfo cpuspeedpanel_info = {
      sizeof (CpuSpeedPanelClass),
      NULL,		/* base_init */
      NULL,		/* base_finalize */
      (GClassInitFunc) cpuspeedpanel_class_init,
      NULL,		/* class_finalize */
      NULL,		/* class_data */
      sizeof (CpuSpeedPanel),
      0,		/* n_preallocs */
      (GInstanceInitFunc) cpuspeedpanel_init,
      NULL,           /* value_table */
    };
    cpuspeedpanel_type = g_type_register_static (GTK_TYPE_FRAME, "CpuSpeedPanel", &cpuspeedpanel_info, 0);
  }

  return cpuspeedpanel_type;
}

enum {
    SPEED_CHANGE_SIGNAL,
    IDLE_CHANGE_SIGNAL,
    LAST_SIGNAL
};

static guint cpuspeedpanel_signals[LAST_SIGNAL];

static void cpuspeedpanel_class_init (CpuSpeedPanelClass *class)
{
    gtkutil_add_signals_to_class ((GtkObjectClass *)class,
				   GTK_STRUCT_OFFSET (CpuSpeedPanelClass, cpuspeedpanel),
				   cpuspeedpanel_signals,
				   "cpuspeed-changed",
				   "cpuidle-changed",
				   (void *)0);
    class->cpuspeedpanel = NULL;
}

static void cpuspeedpanel_init (CpuSpeedPanel *cspanel)
{
    GtkWidget *table;
/*    GtkWidget *hbuttonbox, *button1, *button2; */

    gtk_frame_set_label (GTK_FRAME(cspanel), "Emulation speed");
    gtk_container_set_border_width (GTK_CONTAINER (cspanel), PANEL_BORDER_WIDTH);
    gtk_frame_set_label_align (GTK_FRAME(cspanel), 0.01, 0.5);

    gtkutil_add_table (GTK_WIDGET (cspanel),
	make_label ("Speed"), 1, 1, GTK_FILL,
	cspanel->speed_widget = make_chooser (3, "Maximum", "Approximate 7MHz 68000", "Adjustable"), 2, 1, GTK_EXPAND | GTK_FILL,
	GTKUTIL_ROW_END,
	make_label("Cycles per\ninstruction"), 1, 1, GTK_FILL,
        cspanel->adjust_widget = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (2048, 1, 5121, 1, 512, 1))), 2, 1, GTK_FILL,
	GTKUTIL_ROW_END,

        cspanel->idleenabled_widget = gtk_check_button_new_with_label ("Idle on STOP instruction"), 1, 2, GTK_EXPAND,
	GTKUTIL_ROW_END,
	make_label("Idle rate"), 1, 1, GTK_FILL,
	cspanel->idlerate_widget = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (1, 1, 21, 1, 1, 1))), 2, 1, GTK_FILL,
	GTKUTIL_ROW_END,

	GTKUTIL_TABLE_END
    );

    gtk_scale_set_digits (GTK_SCALE (cspanel->adjust_widget), 0);
    gtk_range_set_update_policy (GTK_RANGE (cspanel->adjust_widget), GTK_UPDATE_DISCONTINUOUS);

    gtk_scale_set_digits (GTK_SCALE (cspanel->idlerate_widget), 0);
    gtk_range_set_update_policy (GTK_RANGE (cspanel->idlerate_widget), GTK_UPDATE_DISCONTINUOUS);


#if 0
    gtk_signal_connect (GTK_OBJECT (cspanel->speed_widget), "selection-changed",
												GTK_SIGNAL_FUNC (on_speed_changed),
												cspanel);
#endif

    gtk_signal_connect (GTK_OBJECT (GTK_COMBO(cspanel->speed_widget)->popwin), "hide",
												GTK_SIGNAL_FUNC (on_speed_changed),
												cspanel);

    gtk_signal_connect (GTK_OBJECT ( GTK_RANGE(cspanel->adjust_widget)->adjustment), "value-changed",
												GTK_SIGNAL_FUNC (on_adjust_changed),
												cspanel);
    gtk_signal_connect (GTK_OBJECT (cspanel->idleenabled_widget), "toggled",
												GTK_SIGNAL_FUNC (on_idleenabled_toggled),
												cspanel);
    gtk_signal_connect (GTK_OBJECT (GTK_RANGE (cspanel->idlerate_widget)->adjustment), "value-changed",
												GTK_SIGNAL_FUNC (on_idlerate_changed),
												cspanel);

    update_state (cspanel);
}

static void update_state (CpuSpeedPanel *cspanel)
{
    //guint speed = GTK_COMBO (cspanel->speed_widget)->choice;
    guint speed = combo_get_choice_num(cspanel->speed_widget);
    guint idleenabled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cspanel->idleenabled_widget));

		kprintf("update_state: GTK_COMBO (cspanel->speed_widget)->choice: %d\n", speed);

    gtk_widget_set_sensitive (cspanel->idleenabled_widget, speed != 1);
    gtk_widget_set_sensitive (cspanel->idlerate_widget, speed != 1 && idleenabled);
    gtk_widget_set_sensitive (cspanel->adjust_widget, speed == 2);

}

static void on_speed_changed (GtkWidget *w, CpuSpeedPanel *cspanel)
{
    //int speed_choice = GTK_COMBO(cspanel->speed_widget)->choice;
    int speed_choice = combo_get_choice_num(cspanel->speed_widget);

    if (speed_choice == 0) {
			cspanel->cpuspeed = -1;
		}
    else if (speed_choice == 1) {
			cspanel->cpuspeed = 0;
		}
    else {
			cspanel->cpuspeed = (guint)GTK_ADJUSTMENT (GTK_RANGE (cspanel->adjust_widget)->adjustment)->value;
		}

    gtk_signal_emit_by_name (GTK_OBJECT(cspanel), "cpuspeed-changed");

    update_state (cspanel);
}

static void on_adjust_changed (GtkWidget *w, CpuSpeedPanel *cspanel)
{
    cspanel->cpuspeed = (guint)GTK_ADJUSTMENT (GTK_RANGE (cspanel->adjust_widget)->adjustment)->value;
    gtk_signal_emit_by_name (GTK_OBJECT(cspanel), "cpuspeed-changed");
}

static void on_idleenabled_toggled (GtkWidget *w, CpuSpeedPanel *cspanel)
{
    cspanel->idleenabled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cspanel->idleenabled_widget));
    if (!cspanel->idleenabled)
	cspanel->cpuidle = 0;
    else
        cspanel->cpuidle = (21 - GTK_ADJUSTMENT (GTK_RANGE (cspanel->idlerate_widget)->adjustment)->value)*15;
    update_state (cspanel);
    gtk_signal_emit_by_name (GTK_OBJECT(cspanel), "cpuidle-changed");
}

static void on_idlerate_changed (GtkWidget *w, CpuSpeedPanel *cspanel)
{
    cspanel->cpuidle = (21 - GTK_ADJUSTMENT (GTK_RANGE (cspanel->idlerate_widget)->adjustment)->value)*15;
    gtk_signal_emit_by_name (GTK_OBJECT(cspanel), "cpuidle-changed");
}

GtkWidget *cpuspeedpanel_new (void)
{
    CpuSpeedPanel *w = CPUSPEEDPANEL (gtk_type_new (cpuspeedpanel_get_type ()));

    return GTK_WIDGET (w);
}

void cpuspeedpanel_set_cpuspeed (CpuSpeedPanel *cspanel, gint cpuspeed) {

	int choice = 0;

	if (cpuspeed == -1)
		choice = 0;
	else if (cpuspeed == 0)
		choice = 1;
	else
		choice = 2;

	kprintf("cpuspeedpanel_set_cpuspeed: gtk_list_select_item(speed_widget %lx): %d\n",cspanel->speed_widget,choice);

   //chooserwidget_set_choice (GTK_COMBO (cspanel->speed_widget), choice);
	gtk_list_select_item (GTK_LIST (GTK_COMBO (cspanel->speed_widget)->list), choice);

	/* manuall call it here, as gtk-mui seems to have problems detecting this change.
	 * gtk-mui bug is in gtk_list.c:gtk_real_list_select_child() ..
	 */
	gtk_signal_emit_by_name (GTK_COMBO(cspanel->speed_widget)->list, "selection-changed");

	if (choice == 2) {
		/* update adjustable speed */
		gtk_adjustment_set_value (GTK_ADJUSTMENT (GTK_RANGE (cspanel->adjust_widget)->adjustment), cpuspeed);
	}

	update_state (cspanel);
}

void cpuspeedpanel_set_cpuidle (CpuSpeedPanel *cspanel, guint cpuidle)
{
    int enable_idle = cpuidle > 0;

    if (enable_idle) {
	cspanel->cpuidle = 21 - (cpuidle / 15);
	gtk_adjustment_set_value (GTK_ADJUSTMENT (GTK_RANGE (cspanel->idlerate_widget)->adjustment), cspanel->cpuidle);
    }
    cspanel->idleenabled = enable_idle;
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cspanel->idleenabled_widget), enable_idle);
}
