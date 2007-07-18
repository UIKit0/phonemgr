
SUBDIRS = icons

uidir = $(datadir)/$(PACKAGE)
ui_DATA = phonemgr.glade

sounddir = $(datadir)/$(PACKAGE)
sound_DATA = alert.wav

schema_in_files = gnome-phone-manager.schemas.in
schemadir = $(GCONF_SCHEMA_FILE_DIR)
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

install-data-local: $(schema_DATA)
if GCONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)" ; then \
	for p in $^ ; do \
		GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $$p 2>&1 > /dev/null; \
	done \
	fi
endif

EXTRA_DIST = $(ui_DATA) $(sound_DATA) $(schema_in_files)

