
AM_CPPFLAGS = \
	$(PHONEMGR_CFLAGS) \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DDATA_DIR="\"$(datadir)/$(PACKAGE)\"" \
	-DVERSION="\"$(VERSION)\"" \
	-DPACKAGE="\"$(PACKAGE)\"" \
	-I$(srcdir)/../libgsm \
	-I.

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
	phonemgr-object.c		\
	phonemgr-object.h

gnome_phone_manager_LDADD = \
	$(PHONEMGR_LIBS) \
	../libgsm/libgsmwrap.la

BUILT_SOURCES = phone-manager-interface.h

phone-manager-interface.h: org_gnome_phone_Manager.xml
	$(DBUS_BIND) --prefix=phonemgr_object --mode=glib-server $< > $@

EXTRA_DIST = gnome-phone-manager.desktop.in org_gnome_phone_Manager.xml
CLEANFILES = $(BUILT_SOURCES)

desktop_in_files = gnome-phone-manager.desktop.in
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

# The test program
noinst_PROGRAMS = test-entry
test_entry_SOURCES = test-entry.c e-phone-entry.c e-phone-entry.h $(CLA_FILES)
test_entry_LDADD = $(PHONEMGR_LIBS)

CLA_FILES = e-contact-entry.h e-contact-entry.c
CLADIR = $(srcdir)/../../contact-lookup-applet/src/
regenerate-built-sources:
	EGGFILES="$(CLA_FILES)" EGGDIR="$(CLADIR)" $(srcdir)/update-from-egg.sh || true

