
AM_CPPFLAGS = \
	$(PHONEMGR_CFLAGS) \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DDATA_DIR="\"$(datadir)/$(PACKAGE)\"" \
	-DVERSION="\"$(VERSION)\"" \
	-DPACKAGE="\"$(PACKAGE)\"" \
	-I$(srcdir)/../libgsm \
	-I$(srcdir)/../libegg/libegg/tray


bin_PROGRAMS = gnome-phone-manager

gnome_phone_manager_SOURCES = \
	main.c icon.c app.h menu.c ui.c \
	connection.c

gnome_phone_manager_LDADD = \
	$(PHONEMGR_LIBS) \
	../libegg/libegg/tray/libeggtray.la \
	../libgsm/libgsmwrap.la

EXTRA_DIST = gnome-phone-manager.desktop.in

desktop_in_files = gnome-phone-manager.desktop.in
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
