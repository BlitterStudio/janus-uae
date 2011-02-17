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
static void on_fpuenabled_toggled (GtkWidget *w, CpuTypePanel *ctpanel);
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

static const char *cpulabels[] = { "68000", "68010", "68ec020", "68020", "68040", "68060", NULL };
static const char *fpulabels[] = { "None", "6888x/internal", NULL };
static const char *acclabels[] = { "Normal", "Compatible", "Cycle Exact", NULL };

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

int make_radio_group_param (const char **label, GtkWidget *tobox,
			      GtkWidget **saveptr, gint t1, gint t2,
			      void (*sigfunc) (void), int count, GSList *group,
			      GtkWidget *parameter);

GtkWidget *make_radio_group_box_param (const char *title, const char **labels,
					GtkWidget **saveptr, int horiz,
					void (*sigfunc) (void), 
					GtkWidget *parameter);

static void cputypepanel_init (CpuTypePanel *ctpanel) {

  GtkWidget *hbox;
  GtkWidget *cpuframe;
  GtkWidget *cpu_widget[10];

  /* hbox for the frames */
  hbox=gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start (GTK_BOX (ctpanel), hbox, FALSE, TRUE, 0);
  gtk_widget_show(hbox);

  cpuframe=make_radio_group_box_1_param("CPU", cpulabels, ctpanel->cpu_widgets, 0, on_cputype_changed, 2, GTK_WIDGET(ctpanel));
  gtk_widget_show(cpuframe);
  gtk_box_pack_start (GTK_BOX (hbox), cpuframe, FALSE, TRUE, 0);

  /* FPU */
  ctpanel->cpu_widget_fpu=make_radio_group_box_param("FPU", fpulabels, ctpanel->cpu_fpu_widgets, 0, (gpointer) on_fpuenabled_toggled, GTK_WIDGET(ctpanel));
  gtk_widget_show(ctpanel->cpu_widget_fpu);
  gtk_box_pack_start (GTK_BOX (hbox), ctpanel->cpu_widget_fpu, FALSE, TRUE, 0);

  /* Accuracy */
  ctpanel->cpu_widget_accuracy=make_radio_group_box_param("Accuracy", acclabels, ctpanel->cpu_accuracy_widgets, 0, (gpointer) on_accuracy_changed, GTK_WIDGET(ctpanel));
  gtk_widget_show(ctpanel->cpu_widget_accuracy);
  gtk_box_pack_start (GTK_BOX (hbox), ctpanel->cpu_widget_accuracy, FALSE, TRUE, 0);
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

  if(cpu > CPULEVEL_68040) {
    cpu=5;
  }

  /* 
   * reset all values, otherwise it may happen, that the new and the default value
   * are the same and on_cputype_changed decides to do nothing. But it should 
   * update all widgets nevertheless.
   */
  ctpanel->addr24bit  = -1;
  ctpanel->cpulevel   = -1;
  ctpanel->fpuenabled = -1;
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
  gint fpu;

  kprintf("update_state: cpu %d \n", ctpanel->cpulevel);

  /* force setting for all !CPULEVEL_680ec20 CPUs */
  if(ctpanel->cpulevel < CPULEVEL_680ec20) {
    fpu=FALSE;
    gtk_widget_set_sensitive (ctpanel->cpu_widget_fpu, FALSE);
  }
  else if(ctpanel->cpulevel > CPULEVEL_680ec20) {
    fpu=TRUE;
    gtk_widget_set_sensitive (ctpanel->cpu_widget_fpu, FALSE);
  }
  else /* (ctpanel->cpulevel == CPULEVEL_680ec20) */ {
    /* let user decide for fpu */
    gtk_widget_set_sensitive (ctpanel->cpu_widget_fpu, TRUE);
    fpu=ctpanel->fpuenabled;

    gtk_signal_handler_block_by_data   (GTK_OBJECT        (ctpanel->cpu_fpu_widgets[fpu]), ctpanel );
    gtk_toggle_button_set_active       (GTK_TOGGLE_BUTTON (ctpanel->cpu_fpu_widgets[fpu]), TRUE);
    gtk_signal_handler_unblock_by_data (GTK_OBJECT        (ctpanel->cpu_fpu_widgets[fpu]), ctpanel );
  }

  ctpanel->fpuenabled=fpu;

  kprintf("update_state: new fpu %d ctpanel->fpuenabled %d\n", fpu, ctpanel->fpuenabled);

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
    /* 68000, 68010, 680ec20, 68020, 68040 */
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

  if(ctpanel->cpulevel < CPULEVEL_68020) {
    ctpanel->addr24bit=TRUE;
  }
  else {
    ctpanel->addr24bit=FALSE;
  }

  kprintf("on_cputype_changed ctpanel->addr24bit: %d\n", ctpanel->addr24bit);

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
 * on_fpuenabled_toggled
 *
 * FPU enabled/disabled
 ******************************/
static void on_fpuenabled_toggled (GtkWidget *me, CpuTypePanel *ctpanel) {

  gint old;

  old=ctpanel->fpuenabled;

  if(me == ctpanel->cpu_fpu_widgets[1]) {
    ctpanel->fpuenabled=TRUE;
  }
  else {
    ctpanel->fpuenabled=TRUE;
  }

  if(old != ctpanel->fpuenabled) {
    gtk_signal_emit_by_name (GTK_OBJECT(ctpanel), "cputype-changed");
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

#if 0
  if (ctpanel->cputype != 2) {
    return;
  }

  if(addr24bit) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctpanel->cpu_addr24bit_widgets[0]), TRUE);
  }
  else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctpanel->cpu_addr24bit_widgets[1]), TRUE);
  }
#endif
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

