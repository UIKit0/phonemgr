AM_CPPFLAGS = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DDATA_DIR="\"$(pkgdatadir)\""			\
	-DVERSION="\"$(VERSION)\""			\
	-DPACKAGE="\"$(PACKAGE)\""			\
	-I$(srcdir)/../libgsm				\
	-I$(builddir)/../libgsm				\
	-I$(srcdir)/../src				\
	$(WARN_CFLAGS)

BLUETOOTH_PLUGIN_SOURCES =		\
	phonemgr.c

if HAVE_BLUETOOTH_PLUGIN
plugindir = $(libdir)/gnome-bluetooth/plugins

plugin_LTLIBRARIES = libphonemgr.la
libphonemgr_la_CFLAGS = $(BLUETOOTH_PLUGIN_CFLAGS)
libphonemgr_la_LDFLAGS = -module -avoid-version
libphonemgr_la_LIBADD = $(BLUETOOTH_PLUGIN_LIBS)
endif

libphonemgr_la_SOURCES = $(BLUETOOTH_PLUGIN_SOURCES)
