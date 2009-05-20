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

  /* radio widget arrays */
  GtkWidget *coherence_widget[2];
  GtkWidget *clipboard_widget[2];

  /*** PUBLIC ***/
};

struct _jIntegrationClass {
  GtkHBoxClass parent_class;
};

GType      jintegration_get_type              (void) G_GNUC_CONST;
GtkWidget* jintegration_new                   (void);


#endif /* __JINTEGRATION_H__ */
