/************************************************************************ 
 *
 * cputypepanel.h
 *
 * Copyright 2011 Oliver Brunner - aros<at>oliver-brunner.de
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


#ifndef __CPUTYPEPANEL_H__
#define __CPUTYPEPANEL_H__

#include <gdk/gdk.h>
#include <gtk/gtkframe.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CPUTYPEPANEL(obj)          GTK_CHECK_CAST (obj, cputypepanel_get_type (), CpuTypePanel)
#define CPUTYPEPANEL_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, cputypepanel_get_type (), CpuTypePanelClass)
#define IS_CPUTYPEPANEL(obj)       GTK_CHECK_TYPE (obj, cputype_panel_get_type ())

typedef struct _CpuTypePanel       CpuTypePanel;
typedef struct _CpuTypePanelClass  CpuTypePanelClass;

struct _CpuTypePanel {

    GtkFrame   frame;
    GtkWidget *cpu_widget_type;
    GtkWidget *cpu_widgets[7];
    GtkWidget *cpu_widget_addr24bit;
    GtkWidget *cpu_addr24bit_widgets[3];
    GtkWidget *cpu_widget_fpu;
    GtkWidget *cpu_fpu_widgets[4];

    GtkWidget *cpu_widget_accuracy;
    GtkWidget *cpu_accuracy_widgets[4];

    guint      cputype;
    guint      cpulevel;
    guint      addr24bit;
    guint      fpuenabled;
    guint      compatible;
    guint      cycleexact;
};

struct _CpuTypePanelClass
{
  GtkFrameClass parent_class;

  void (* cputypepanel) (CpuTypePanel *cputypepanel );
};

guint		cputypepanel_get_type	    (void);
GtkWidget*	cputypepanel_new	    (void);
void		cputypepanel_set_cpulevel   (CpuTypePanel *ctpanel, guint cpulevel);
void		cputypepanel_set_addr24bit  (CpuTypePanel *ctpanel, guint addr24bit);
guint		cputypepanel_get_cpulevel   (CpuTypePanel *ctpanel);
guint		cputypepanel_get_addr24bit  (CpuTypePanel *ctpanel);
void            cputypepanel_set_accuracy   (CpuTypePanel *ctpanel, gboolean compatible, gboolean cycleexact);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CPUTYPEPANEL_H__ */
