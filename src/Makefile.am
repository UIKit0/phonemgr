
AM_CPPFLAGS = \
	$(PHONEMGR_CFLAGS) \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DDATA_DIR="\"$(datadir)/$(PACKAGE)\"" \
	-DVERSION="\"$(VERSION)\"" \
	-DPACKAGE="\"$(PACKAGE)\"" \
	-I$(srcdir)/../libgsm

bin_PROGRAMS = gnome-phone-manager

gnome_phone_manager_SOURCES =		\
	main.c				\
	icon.c				\
	app.h				\
	menu.c				\
	ui.c				\
	connection.c			\
	e-phone-entry.c			\
	e-phone-entry.h			\
	$(CLA_FILES)

gnome_phone_manager_LDADD = \
	$(PHONEMGR_LIBS) \
	../libgsm/libgsmwrap.la

EXTRA_DIST = gnome-phone-manager.desktop.in

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

