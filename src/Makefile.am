
bin_PROGRAMS = gnome-phone-manager

gnome_phone_manager_SOURCES = \
	main.c

gnome_phone_manager_LDADD = \
	$(PHONEMGR_LIBS) \
	../libegg/libegg/tray/libeggtray.la \
	../libgsm/libgsmwrap.la

INCLUDES = \
	$(PHONEMGR_CFLAGS)
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DDATA_DIR="\"$(datadir)/$(PACKAGE)\""

