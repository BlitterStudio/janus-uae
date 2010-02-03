/************************************************************************ 
 *
 * integration.h
 *
 * GTK notebook page for coherent mode settings
 *
 * Copyright 2009 Oliver Brunner - aros<at>oliver-brunner.de
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
 ***********************************************************************/


#ifndef __JINTEGRATION_H__
#define __JINTEGRATION_H__

#include <gtk/gtkhbox.h>

#define TYPE_JINTEGRATION                  (jintegration_get_type ())
#define JINTEGRATION(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_JINTEGRATION, jIntegration))
#define JINTEGRATION_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_JINTEGRATION, jIntegrationClass))
#define IS_JINTEGRATION(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_JINTEGRATION))
#define IS_JINTEGRATION_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_JINTEGRATION))
#define JINTEGRATION_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_JINTEGRATION, jIntegrationClass))


typedef struct _jIntegration       jIntegration;
typedef struct _jIntegrationClass  jIntegrationClass;

struct _jIntegration {
  GtkHBox object;             /* parent object */

  /*** PRIVATE ***/

  GtkWidget *combo_coherence;
  GtkWidget *label_coherence;

  GtkWidget *combo_mouse;
  GtkWidget *label_mouse;

  GtkWidget *combo_clip;
  GtkWidget *label_clip;

  GtkWidget *combo_launch;
  GtkWidget *label_launch;

  /*** PUBLIC ***/

  gboolean coherence;
  gboolean mouse;
  gboolean clipboard;
};

struct _jIntegrationClass {
  GtkHBoxClass parent_class;
};

GType      jintegration_get_type              (void) G_GNUC_CONST;
GtkWidget* jintegration_new                   (void);


#endif /* __JINTEGRATION_H__ */
