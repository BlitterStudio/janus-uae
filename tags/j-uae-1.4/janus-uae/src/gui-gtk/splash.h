/************************************************************************ 
 *
 * splash.h
 *
 * GTK window class for splash screens
 *
 * Copyright 2012 Oliver Brunner - aros<at>oliver-brunner.de
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


#ifndef __JSPLASH_H__
#define __JSPLASH_H__

#include <gtk/gtkhbox.h>

GType jsplash_get_type (void);

#define TYPE_JSPLASH                  (jsplash_get_type ())
#define JSPLASH(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_JSPLASH, jSplash))
#define JSPLASH_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_JSPLASH, jSplashClass))
#define IS_JSPLASH(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_JSPLASH))
#define IS_JSPLASH_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_JSPLASH))
#define JSPLASH_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_JSPLASH, jSplashClass))


typedef struct _jSplash       jSplash;
typedef struct _jSplashClass  jSplashClass;

struct _jSplash {
  GtkWindow object;             /* parent object */

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
  gboolean launch;
};

struct _jSplashClass {
  GtkWindowClass parent_class;
};

GType      jsplash_get_type              (void) G_GNUC_CONST;
GtkWidget* jsplash_new                   (void);


#endif /* __JSPLASH_H__ */
