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

#define CPULEVEL_68000      0
#define CPULEVEL_68010      1
#define CPULEVEL_68020      2
#define CPULEVEL_68020FPU   3
#define CPULEVEL_68040      4
#define CPULEVEL_68060      6

struct _CpuTypePanel {

  /**** private ****/
  GtkFrame   frame;

  GtkWidget *cpu_widget_addr24bit;
  GtkWidget *addr24bit_widgets[3];

  GtkWidget *cpu_widget_accuracy;
  GtkWidget *cpu_accuracy_widgets[4];

  /**** private new ****/
  GtkWidget *cpu_widgets[8];

  /**** public ****/

  /* cpulevel:                   (FPU)   
   *  0: 68000                   undef   
   *  1: 68010                   undef
   *  2: 68020                   FALSE
   *  3: 68020+FPU               TRUE
   *  4: 68040                   TRUE
   *  5: not used (68060)        TRUE
   *  6: 68060                   TRUE
   */
  gint      cpulevel;
  gint      addr24bit;
  gint      compatible;
  gint      cycleexact;

};

struct _CpuTypePanelClass
{
  GtkFrameClass parent_class;

  void (* cputypepanel) (CpuTypePanel *cputypepanel );
};

guint		cputypepanel_get_type	    (void);
GtkWidget*	cputypepanel_new	    (void);
void		cputypepanel_set_cpulevel   (CpuTypePanel *ctpanel, gint cpulevel);
void		cputypepanel_set_addr24bit  (CpuTypePanel *ctpanel, guint addr24bit);
guint		cputypepanel_get_cpulevel   (CpuTypePanel *ctpanel);
guint		cputypepanel_get_addr24bit  (CpuTypePanel *ctpanel);
void            cputypepanel_set_accuracy   (CpuTypePanel *ctpanel, gboolean compatible, gboolean cycleexact);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CPUTYPEPANEL_H__ */
