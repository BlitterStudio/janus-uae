/************************************************************************ 
 *
 * cpuspeedpanel.c
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

#include "cputypepanel.h"
#include "chooserwidget.h"
#include "util.h"

/****************************************************************************************/

static void cputypepanel_init (CpuTypePanel *pathent);
static void cputypepanel_class_init (CpuTypePanelClass *class);
static void update_state (CpuTypePanel *ctpanel);
static void on_cputype_changed (GtkWidget *w, CpuTypePanel *ctpanel);
static void on_addr24bit_toggled (GtkWidget *w, CpuTypePanel *ctpanel);
static void on_fpuenabled_toggled (GtkWidget *w, CpuTypePanel *ctpanel);
static void on_accuracy_changed (GtkWidget *w, CpuTypePanel *ctpanel);

/****************************************************************************************/

enum {
    TYPE_CHANGE_SIGNAL,
    ADDR24_CHANGE_SIGNAL,
    LAST_SIGNAL
};

static guint cputypepanel_signals[LAST_SIGNAL];

static const char *cpulabels[] = { "68000", "68010", "68020", "68040", "68060", NULL };
static const char *fpulabels[] = { "None", "6888x/internal", NULL };
static const char *bitlabels[] = { "24-bit", "32-bit", NULL };
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
	//cputypepanel_type = gtk_type_unique (gtk_frame_get_type (), &cputypepanel_info);
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
				   (void*)0);
    class->cputypepanel = NULL;
}

GtkWidget *cputypepanel_new (void) {

    CpuTypePanel *w = CPUTYPEPANEL (gtk_type_new (cputypepanel_get_type ()));

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

  /* hbox for the frames */
  hbox=gtk_hbox_new(TRUE, 5);
  gtk_box_pack_start (GTK_BOX (ctpanel), hbox, FALSE, TRUE, 0);
  gtk_widget_show(hbox);

  /* 680x0 */
  ctpanel->cpu_widget_type=make_radio_group_box_param("CPU", cpulabels, ctpanel->cpu_widgets, 0, (gpointer) on_cputype_changed, GTK_WIDGET(ctpanel));
  gtk_widget_show(ctpanel->cpu_widget_type);
  gtk_box_pack_start (GTK_BOX (hbox), ctpanel->cpu_widget_type, FALSE, TRUE, 0);

  /* FPU */
  ctpanel->cpu_widget_fpu=make_radio_group_box_param("FPU", fpulabels, ctpanel->cpu_fpu_widgets, 0, (gpointer) on_fpuenabled_toggled, GTK_WIDGET(ctpanel));
  gtk_widget_show(ctpanel->cpu_widget_fpu);
  gtk_box_pack_start (GTK_BOX (hbox), ctpanel->cpu_widget_fpu, FALSE, TRUE, 0);

  /* Addressing */
  ctpanel->cpu_widget_addr24bit=make_radio_group_box_param("Addressing", bitlabels, ctpanel->cpu_addr24bit_widgets, 0, (gpointer) on_addr24bit_toggled, GTK_WIDGET(ctpanel));
  gtk_widget_show(ctpanel->cpu_widget_addr24bit);
  gtk_box_pack_start (GTK_BOX (hbox), ctpanel->cpu_widget_addr24bit, FALSE, TRUE, 0);

  /* Accuracy */
  ctpanel->cpu_widget_accuracy=make_radio_group_box_param("Accuracy", acclabels, ctpanel->cpu_accuracy_widgets, 0, (gpointer) on_accuracy_changed, GTK_WIDGET(ctpanel));
  gtk_widget_show(ctpanel->cpu_widget_accuracy);
  gtk_box_pack_start (GTK_BOX (hbox), ctpanel->cpu_widget_accuracy, FALSE, TRUE, 0);

  update_state (ctpanel);
}

/********************
 * update_state
 *
 * updates fpu/addr
 * widgets
 *******************/
static void update_state (CpuTypePanel *ctpanel) {
  guint cpu    = ctpanel->cputype;
  guint addr24 = ctpanel->addr24bit;
  guint fpu    = ctpanel->fpuenabled;

  switch (cpu) {
      case 0:
      case 1:
	  addr24 = 1;
	  fpu    = 0;
	  break;
      case 3:
      case 4:
	  addr24 = 0;
	  fpu    = 1;
	  break;
  }

  /* disable all widget, which are not possible for this CPU */
  gtk_widget_set_sensitive (ctpanel->cpu_widget_addr24bit, cpu == 2);
  gtk_widget_set_sensitive (ctpanel->cpu_widget_fpu, cpu == 2);

  if (fpu != ctpanel->fpuenabled) {
      ctpanel->fpuenabled = fpu;
      gtk_signal_handler_block_by_data (GTK_OBJECT (ctpanel->cpu_fpu_widgets[fpu]), ctpanel );
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ctpanel->cpu_fpu_widgets[fpu]), TRUE);
      gtk_signal_handler_unblock_by_data (GTK_OBJECT (ctpanel->cpu_fpu_widgets[fpu]), ctpanel );
  }

  if (addr24 != ctpanel->addr24bit) {
      ctpanel->addr24bit  = addr24;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctpanel->cpu_addr24bit_widgets[addr24]), TRUE);
      //gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ctpanel->addr24bit_widget), addr24);
  }

  if(cpu==0) {
    /* 68000 */
    gtk_widget_set_sensitive (ctpanel->cpu_widget_accuracy, TRUE);
  }
  else {
    /* not adjustable */
    gtk_widget_set_sensitive (ctpanel->cpu_widget_accuracy, FALSE);
      /* TODO ?? */
    //chooserwidget_set_choice (CHOOSERWIDGET (ctpanel->accuracy_widget), 0);
  }
}

static void on_cputype_changed (GtkWidget *me, CpuTypePanel *ctpanel) {

  guint type;

  if(!ctpanel) {
    /* can happen durint init? */
    return;
  }

  type=0;
  while(ctpanel->cpu_widgets[type] && me!=ctpanel->cpu_widgets[type]) {
    type++;
  }

  if(type == ctpanel->cputype) {
    kprintf("on_cputype_changed: 1a --------> do nothing\n");
    return;
  }

  ctpanel->cputype=type;

  switch (ctpanel->cputype) {
    case 0:  ctpanel->cpulevel = 0; break;
    case 1:  ctpanel->cpulevel = 1; break;
    case 2:  ctpanel->cpulevel = 2 + (ctpanel->fpuenabled!=0); break;
    case 3:  ctpanel->cpulevel = 4 ; break;
    case 4:
    default: ctpanel->cpulevel = 6; break;
  }

 //ctpanel->cputype = type;

  update_state (ctpanel);
  gtk_signal_emit_by_name (GTK_OBJECT(ctpanel), "cputype-changed");

}

static void on_addr24bit_toggled (GtkWidget *me, CpuTypePanel *ctpanel) {

  if(me == ctpanel->cpu_addr24bit_widgets[0]) {
    /* 24 bit */
    ctpanel->addr24bit=1;
  }
  else {
    ctpanel->addr24bit=0;
  }

  gtk_signal_emit_by_name (GTK_OBJECT(ctpanel), "addr24bit-changed");
}

/******************************
 * on_fpuenabled_toggled
 *
 * FPU enabled/disabled
 ******************************/
static void on_fpuenabled_toggled (GtkWidget *me, CpuTypePanel *ctpanel) {

  guint old;

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
  guint old_comp=ctpanel->compatible;
  guint old_cycl=ctpanel->cycleexact;

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

void cputypepanel_set_cpulevel (CpuTypePanel *ctpanel, guint cpulevel) {

    guint cputype; 
    guint fpu = ctpanel->fpuenabled;

    ctpanel->cpulevel=cpulevel;

    switch (cpulevel) {
      case 0:  cputype = 0; break;
      case 1:  cputype = 1; break;
      case 2:  cputype = 2; fpu = 0; break;
      case 3:  cputype = 2; fpu = 1; break;
      case 4:  cputype = 3; fpu = 1; break;
      case 6:  cputype = 4; fpu = 1; break;
      default: cputype = 0;
    }

    ctpanel->cputype = cputype;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctpanel->cpu_widgets[cputype]), TRUE);

    if (fpu != ctpanel->fpuenabled) {
      ctpanel->fpuenabled = fpu;
      /* TODO?? */
      if(!fpu) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctpanel->cpu_widgets[0]), TRUE);
      }
      else {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctpanel->cpu_widgets[1]), TRUE);
      }
      //gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ctpanel->fpuenabled_widget), fpu);
    }

    update_state (ctpanel);
}

guint cputypepanel_get_addr24bit (CpuTypePanel *ctpanel) {

    return ctpanel->addr24bit;
}

void cputypepanel_set_addr24bit (CpuTypePanel *ctpanel, guint addr24bit) {

  if (ctpanel->cputype != 2) {
    return;
  }

  if(addr24bit) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctpanel->cpu_addr24bit_widgets[0]), TRUE);
  }
  else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctpanel->cpu_addr24bit_widgets[1]), TRUE);
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

