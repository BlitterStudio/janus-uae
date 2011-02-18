/************************************************************************ 
 *
 * cputypepanel.c
 *
 * Copyright 2003-2004 Richard Drummond
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

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"

#include "cputypepanel.h"
#include "chooserwidget.h"
#include "util.h"

/****************************************************************************************/

static void cputypepanel_init (CpuTypePanel *pathent);
static void cputypepanel_class_init (CpuTypePanelClass *class);
static void update_state (CpuTypePanel *ctpanel);
static void on_cputype_changed (GtkWidget *w, CpuTypePanel *ctpanel);
static void on_addr24enabled_toggled (GtkWidget *me, CpuTypePanel *ctpanel);
static void on_accuracy_changed (GtkWidget *w, CpuTypePanel *ctpanel);

static void read_prefs (CpuTypePanel *ctpanel);

/****************************************************************************************/

enum {
    TYPE_CHANGE_SIGNAL,
    ADDR24_CHANGE_SIGNAL,
    READ_PREFS,
    LAST_SIGNAL
};

static guint cputypepanel_signals[LAST_SIGNAL];

static const char *cpulabels[]       = { "68000", "68010", "68020", "68020/68881", "68040", "68060", NULL };
static const char *addr24bitlabels[] = { "24-bit addressing", "32-bit addressing", NULL };
static const char *acclabels[]       = { "Normal", "Compatible", "Cycle Exact", NULL };

/****************************************************************************************/

guint cputypepanel_get_type () {

    static guint cputypepanel_type = 0;

    if (!cputypepanel_type) {
	static const GtkTypeInfo cputypepanel_info = {
	    (char *) "CpuTypePanel",
	    sizeof (CpuTypePanel),
	    sizeof (CpuTypePanelClass),
	    (GtkClassInitFunc) cputypepanel_class_init,
	    (GtkObjectInitFunc) cputypepanel_init,
	    NULL,
	    NULL,
	    (GtkClassInitFunc) NULL
	};
	cputypepanel_type = gtk_type_unique (gtk_box_get_type (), &cputypepanel_info);
    }
    return cputypepanel_type;
}

static void cputypepanel_class_init (CpuTypePanelClass *class) {

    gtkutil_add_signals_to_class ((GtkObjectClass *)class,
				   GTK_STRUCT_OFFSET (CpuTypePanelClass, cputypepanel),
				   cputypepanel_signals,
				   "cputype-changed",
				   "addr24bit-changed",
				   "read-prefs",
				   (void*)0);
    class->cputypepanel = NULL;
}

GtkWidget *cputypepanel_new (void) {

  CpuTypePanel *w = CPUTYPEPANEL (gtk_type_new (cputypepanel_get_type ()));

  gtk_signal_connect (GTK_OBJECT (w), "read-prefs",
			  GTK_SIGNAL_FUNC (read_prefs),
			  NULL);

  return GTK_WIDGET (w);
}

/****************************************************************************************/

static void cputypepanel_init (CpuTypePanel *ctpanel) {

  GtkWidget *cpuframe;
  GtkWidget *table;

  cpuframe=make_radio_group_box_1_param("CPU", cpulabels, ctpanel->cpu_widgets, 0, (gpointer) on_cputype_changed, 2, GTK_WIDGET(ctpanel));
  gtk_widget_show(cpuframe);

  /* 24/32 bit */
  ctpanel->cpu_widget_addr24bit=make_radio_group_box_param("Addressing", addr24bitlabels, ctpanel->addr24bit_widgets, 0, (gpointer) on_addr24enabled_toggled, GTK_WIDGET(ctpanel));
  gtk_widget_show(ctpanel->cpu_widget_addr24bit);

  /* Accuracy */
  ctpanel->cpu_widget_accuracy=make_radio_group_box_param("Accuracy", acclabels, ctpanel->cpu_accuracy_widgets, 0, (gpointer) on_accuracy_changed, GTK_WIDGET(ctpanel));
  gtk_widget_show(ctpanel->cpu_widget_accuracy);

  /* table for the frames */
  table=gtk_table_new(3,1,FALSE);
  gtk_widget_show(table);

  gtk_table_attach_defaults(GTK_TABLE(table), cpuframe,                      0, 1, 0, 1);
  gtk_table_attach_defaults(GTK_TABLE(table), ctpanel->cpu_widget_addr24bit, 1, 2, 0, 1);
  gtk_table_attach_defaults(GTK_TABLE(table), ctpanel->cpu_widget_accuracy,  2, 3, 0, 1);

  gtk_container_add (GTK_CONTAINER (ctpanel), table);
}

/****************************************************************************************/

/* 
 * read prefs
 *
 * update all widgets to the current prefs state
 *
 * called through the signal "read-prefs"
 *
 */
static void read_prefs (CpuTypePanel *ctpanel) {

  guint cpu;

  kprintf("read_prefs..\n");

  cpu=currprefs.cpu_level;

  kprintf("=======> %d\n", currprefs.cpu_level);

  if(cpu > CPULEVEL_68040) {
    cpu=5;
  }

  /* 
   * reset all values, otherwise it may happen, that the new and the default value
   * are the same and on_cputype_changed decides to do nothing. But it should 
   * update all widgets nevertheless.
   */
  ctpanel->addr24bit  = currprefs.address_space_24;
  ctpanel->cpulevel   = -1;
  //ctpanel->compatible = -1;
  //ctpanel->cycleexact = -1;

  /* set cpu state */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ctpanel->cpu_widgets[cpu]), TRUE);
  on_cputype_changed (ctpanel->cpu_widgets[cpu], ctpanel);

}

/****************************************************************************************/
/*
 * update_state
 *
 * updates fpu/accuracy widgets dependent on cpulevel
 *
 */
static void update_state (CpuTypePanel *ctpanel) {
  gint addr24bit;
  gint old_addr24bit;

  kprintf("update_state: cpu %d \n", ctpanel->cpulevel);

  old_addr24bit=ctpanel->addr24bit;

  /* force setting for all !CPULEVEL_680ec20 CPUs */
  if(ctpanel->cpulevel < CPULEVEL_68020) {
    addr24bit=TRUE;
    kprintf("update_state 1 addr24bit: %d\n", addr24bit);
    gtk_widget_set_sensitive (ctpanel->cpu_widget_addr24bit, FALSE);
  }
  else if(ctpanel->cpulevel > CPULEVEL_68020FPU) {
    addr24bit=FALSE;
    kprintf("update_state 2 addr24bit: %d\n", addr24bit);
    gtk_widget_set_sensitive (ctpanel->cpu_widget_addr24bit, FALSE);
  }
  else {
    /* CPULEVEL_68020(w/o FPU): user can decide 24bit */
    gtk_widget_set_sensitive (ctpanel->cpu_widget_addr24bit, TRUE);
    addr24bit=ctpanel->addr24bit;
    kprintf("update_state 3 addr24bit: %d\n", addr24bit);
  }

  kprintf("ctpanel->addr24bit: ct->addr24bit %d addr24bit %d old_addr24bit %d\n", ctpanel->addr24bit, addr24bit, old_addr24bit);
  ctpanel->addr24bit=addr24bit;

  //if(old_addr24bit != ctpanel->addr24bit) {
    kprintf("ctpanel->addr24bit: old_addr24bit != ctpanel->addr24bit\n");
    gtk_signal_handler_block_by_data   (GTK_OBJECT (ctpanel->addr24bit_widgets[addr24bit]), ctpanel );
    if(addr24bit) {
      kprintf("ctpanel->addr24bit: ctpanel->addr24bit_widgets[0], TRUE\n");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ctpanel->addr24bit_widgets[0]), TRUE);
    }
    else {
      kprintf("ctpanel->addr24bit: ctpanel->addr24bit_widgets[1], TRUE\n");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ctpanel->addr24bit_widgets[1]), TRUE);
    }
    gtk_signal_handler_unblock_by_data (GTK_OBJECT (ctpanel->addr24bit_widgets[addr24bit]), ctpanel );
  //}

  kprintf("update_state: new addr24: %d\n", addr24bit);

  if(ctpanel->cpulevel == CPULEVEL_68000) {
    gtk_widget_set_sensitive (ctpanel->cpu_widget_accuracy, TRUE);
  }
  else {
    /* not adjustable */
    gtk_widget_set_sensitive (ctpanel->cpu_widget_accuracy, FALSE);
      /* TODO ?? */
    //chooserwidget_set_choice (CHOOSERWIDGET (ctpanel->accuracy_widget), 0);
  }
}

/*
 * on_cputype_changed
 *
 * user clicked on one of the cpu widgets
 */
static void on_cputype_changed (GtkWidget *me, CpuTypePanel *ctpanel) {

  gint type;
  gint old_24bit;
  gint old_cpulevel;

  kprintf("on_cputype_changed\n");

  if(!ctpanel) {
    /* can happen durint init? */
    kprintf("on_cputype_changed: ctpanel is NULL??\n");
    return;
  }

  old_cpulevel=ctpanel->cpulevel;

  kprintf("on_cputype_changed old_cpulevel: %d\n", old_cpulevel);

  /* find clicked widget */
  type=0;
  while(ctpanel->cpu_widgets[type] && me!=ctpanel->cpu_widgets[type]) {
    type++;
  }

  kprintf("on_cputype_changed type: %d\n", type);

  if(type < 5) {
    /* 68000, 68010, 68020, 68020/FPU, 68040 */
    ctpanel->cpulevel = type;
  }
  else {
    /* all above is 68060 */
    ctpanel->cpulevel = CPULEVEL_68060;
  }

  kprintf("on_cputype_changed ctpanel->cpulevel: %d\n", ctpanel->cpulevel);

  /* do we have a new CPU ? */
  if(old_cpulevel == ctpanel->cpulevel) {
    kprintf("on_cputype_changed do nothing\n");
    return;
  }

  old_24bit=ctpanel->addr24bit;

  /* update_state changes ctpanel->addr24bit, if necessary */
  update_state (ctpanel);

  /* we have a new cpu */
  gtk_signal_emit_by_name (GTK_OBJECT(ctpanel), "cputype-changed");
  kprintf("on_cputype_changed cputype-changed\n");

  /* do we have a new 24bit mode ? */
  if( old_24bit != ctpanel->addr24bit) {
    gtk_signal_emit_by_name (GTK_OBJECT(ctpanel), "addr24bit-changed"); 
    kprintf("on_cputype_changed addr24bit-changed\n");
  }
}

/******************************
 * on_addr24enabled_toggled
 *
 * addr24 enabled/disabled
 ******************************/
static void on_addr24enabled_toggled (GtkWidget *me, CpuTypePanel *ctpanel) {

  gint old;

  old=ctpanel->addr24bit;

  if(me == ctpanel->addr24bit_widgets[1]) {
    kprintf("on_addr24enabled_toggled: FALSE\n");
    ctpanel->addr24bit=FALSE;
  }
  else {
    ctpanel->addr24bit=TRUE;
    kprintf("on_addr24enabled_toggled: TRUE\n");
  }

  if(old != ctpanel->addr24bit) {
    gtk_signal_emit_by_name (GTK_OBJECT(ctpanel), "addr24bit-changed");
  }
}

/******************************
 * on_accuracy_changed
 *
 * compatible/cycle exact 
 * changed
 ******************************/
static void on_accuracy_changed (GtkWidget *me, CpuTypePanel *ctpanel) {
  gint old_comp=ctpanel->compatible;
  gint old_cycl=ctpanel->cycleexact;

  if (me == ctpanel->cpu_accuracy_widgets[0]) {
    /* normal */
    ctpanel->compatible = 0;
    ctpanel->cycleexact = 0;
  } else if (me == ctpanel->cpu_accuracy_widgets[1]) {
    /* compatible */
    ctpanel->compatible = 1;
    ctpanel->cycleexact = 0;
  } else {
    /* cycleexact */
    ctpanel->compatible = 0;
    ctpanel->cycleexact = 1;
  }

  if( (ctpanel->compatible != old_comp) ||
      (ctpanel->cycleexact != old_cycl) ) {

    gtk_signal_emit_by_name (GTK_OBJECT(ctpanel), "cputype-changed");
  }
}

void cputypepanel_set_cpulevel (CpuTypePanel *ctpanel, gint cpulevel) {

  if(cpulevel > CPULEVEL_68040) {
    cpulevel=CPULEVEL_68060;
  }

  if(cpulevel == ctpanel->cpulevel) {
    return;
  }

  if(ctpanel->cpulevel < CPULEVEL_68060)  {
    on_cputype_changed (ctpanel->cpu_widgets[cpulevel], ctpanel);
  }
  else {
    on_cputype_changed (ctpanel->cpu_widgets[5], ctpanel);
  }
}

guint cputypepanel_get_addr24bit (CpuTypePanel *ctpanel) {

    return ctpanel->addr24bit;
}

void cputypepanel_set_addr24bit (CpuTypePanel *ctpanel, guint addr24bit) {

  /* TODO */

  if( (ctpanel->cpulevel<CPULEVEL_68020) || (ctpanel->cpulevel>CPULEVEL_68020FPU) ) {
    return;
  }

  if(addr24bit) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctpanel->addr24bit_widgets[0]), TRUE);
  }
  else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctpanel->addr24bit_widgets[1]), TRUE);
  }
}

/************************************************
 * cputypepanel_set_accuracy
 *
 * both prefs.compatible and prefs.cycleexact
 * have an influence on our widgets
 ************************************************/
void cputypepanel_set_accuracy (CpuTypePanel *ctpanel, gboolean compatible, gboolean cycleexact) {

  kprintf("cputypepanel_set_accuracy(%d, %d)\n", compatible, cycleexact);

  if(!cycleexact && !compatible) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctpanel->cpu_accuracy_widgets[0]), TRUE);
    return;
  }

  if(compatible) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctpanel->cpu_accuracy_widgets[1]), TRUE);
    return;
  }

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctpanel->cpu_accuracy_widgets[2]), TRUE);
  return;
}

