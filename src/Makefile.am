
AM_CPPFLAGS = \
	$(PHONEMGR_CFLAGS) \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DDATA_DIR="\"$(pkgdatadir)\"" \
	-DVERSION="\"$(VERSION)\"" \
	-DPACKAGE="\"$(PACKAGE)\"" \
	-I$(srcdir)/../libgsm \
	-I$(builddir)/../libgsm \
	-I$(builddir)/src

bin_PROGRAMS = gnome-phone-manager

gnome_phone_manager_SOURCES =		\
	phone-manager-interface.h	\
	main.c				\
	icon.c				\
	app.h				\
	menu.c				\
	ui.c				\
	connection.c			\
	e-phone-entry.c			\
	e-phone-entry.h			\
	$(CLA_FILES)			\
	$(MARSHALFILES)			\
	phonemgr-object.c		\
	phonemgr-object.h		\
	phonemgr-chooser-button.c	\
	phonemgr-chooser-button.h	\
	gconf-bridge.c			\
	gconf-bridge.h

gnome_phone_manager_LDADD = \
	$(PHONEMGR_LIBS) \
	../libgsm/libgsmwrap.la

BUILT_SOURCES = phone-manager-interface.h

phone-manager-interface.h: org_gnome_phone_Manager.xml
	$(DBUS_BIND) --prefix=phonemgr_object --mode=glib-server $< > $@

desktop_in_files = gnome-phone-manager.desktop.in
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

# The test program
noinst_PROGRAMS = test-entry
test_entry_SOURCES = test-entry.c e-phone-entry.c e-phone-entry.h $(CLA_FILES) $(MARSHALFILES)
test_entry_LDADD = $(PHONEMGR_LIBS)

MARSHALFILES = econtactentry-marshal.c econtactentry-marshal.h
BUILT_SOURCES = $(MARSHALFILES)

econtactentry-marshal.c: econtactentry-marshal.h
	( $(GLIB_GENMARSHAL) --prefix=econtactentry_marshal $(srcdir)/econtactentry-marshal.list --header --body > econtactentry-marshal.c )
econtactentry-marshal.h: econtactentry-marshal.list
	( $(GLIB_GENMARSHAL) --prefix=econtactentry_marshal $(srcdir)/econtactentry-marshal.list --header > econtactentry-marshal.h )

CLA_FILES = e-contact-entry.h e-contact-entry.c
CLADIR = $(srcdir)/../../contact-lookup-applet/src/
GCONF_BRIDGE_FILES = gconf-bridge.h gconf-bridge.c
GCONF_BRIDGE_DIR = $(srcdir)/../../libgconf-bridge/libgconf-bridge/
regenerate-built-sources:
	EGGFILES="$(CLA_FILES) econtactentry-marshal.list" EGGDIR="$(CLADIR)" $(srcdir)/update-from-egg.sh || true
	EGGFILES="$(GCONF_BRIDGE_FILES)" EGGDIR="$(GCONF_BRIDGE_DIR)" $(srcdir)/update-from-egg.sh || true

EXTRA_DIST = gnome-phone-manager.desktop.in org_gnome_phone_Manager.xml econtactentry-marshal.list
CLEANFILES = $(BUILT_SOURCES) $(desktop_DATA)

