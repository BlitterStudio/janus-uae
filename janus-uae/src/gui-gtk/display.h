#ifndef __JDISPLAY_H__
#define __JDISPLAY_H__

#include <gtk/gtkhbox.h>

#define TYPE_JDISPLAY                  (jdisplay_get_type ())
#define JDISPLAY(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_JDISPLAY, jDisplay))
#define JDISPLAY_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_JDISPLAY, jDisplayClass))
#define IS_JDISPLAY(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_JDISPLAY))
#define IS_JDISPLAY_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_JDISPLAY))
#define JDISPLAY_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_JDISPLAY, jDisplayClass))


typedef struct _jDisplay       jDisplay;
typedef struct _jDisplayClass  jDisplayClass;

struct _jDisplay {
  GtkHBox object;             /* parent object */

  /*** PRIVATE ***/

  /* combo widget */
  GtkWidget *screen_resolutions;
  GList     *res_items;

  /* entry widget */
  GtkWidget *screen_width;
  GtkWidget *screen_height;

  /* checkmark widgets */
  GtkWidget *settings_correctaspect;
  GtkWidget *settings_lores;
  GtkWidget *settings_fullscreen;
  GtkWidget *settings_fullscreen_rtg;

  GtkWidget *emuspeed;

  /* radio widget arrays */
  GtkWidget *linemode_widget[3];
  GtkWidget *centering_widget[4];
  GtkWidget *chipset_widget[5];
  GtkWidget *pal_ntsc_widget[2];

  /*** PUBLIC ***/

  guint      gfx_width_win, gfx_height_win; 
  guint      gfx_width_fs, gfx_height_fs;
  guint      chipset_mask;/* ocs, ecs_agnus, ecs_denise, ecs, aga*/
  guint      ntscmode;
  guint      gfx_linedbl; /* none, double, scanlines */
  guint      gfx_correct_aspect;
  guint      gfx_lores;
  guint      gfx_fullscreen_amiga;
  guint      gfx_fullscreen_p96;
  guint      gfx_xcenter;
  guint      gfx_ycenter;

};

struct _jDisplayClass {
  GtkHBoxClass parent_class;
};

GType      jdisplay_get_type              (void) G_GNUC_CONST;
GtkWidget* jdisplay_new                   (void);


#endif /* __JDISPLAY_H__ */
