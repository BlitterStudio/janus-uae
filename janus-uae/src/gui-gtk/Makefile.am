INCLUDES =  -I$(top_srcdir)/src/include -I$(top_builddir)/src -I$(top_srcdir)/src -I$(top_srcdir) -I/usr/local/aros/include -I/usr/local/aros/include/glib-2.0/glib -I/usr/local/aros/include/glib-2.0


AM_CFLAGS = -DGTKMUI

noinst_LIBRARIES = libguidep.a

libguidep_a_SOURCES = gtkui.c util.c chooserwidget.c cputypepanel.c \
		      cpuspeedpanel.c floppyfileentry.c led.c \
		      chipsettypepanel.c chipsetspeedpanel.c \
		      display.c integration.c

noinst_HEADERS = util.h chooserwidget.h cputypepanel.h cpuspeedpanel.h \
		 floppyfileentry.h led.h chipsettypepanel.h \
		 chipsetspeedpanel.h display.h integration.h

