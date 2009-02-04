AM_CPPFLAGS = \
	$(PHONEMGR_CFLAGS)

noinst_LTLIBRARIES = libgconf-bridge.la

libgconf_bridge_la_SOURCES =	\
	gconf-bridge.c	\
	gconf-bridge.h

EGGDIR=$(srcdir)/../../../libgconf-bridge/libgconf-bridge/
EGGFILES=gconf-bridge.c gconf-bridge.h
regenerate-built-sources:
	EGGFILES="$(EGGFILES)" EGGDIR="$(EGGDIR)" $(srcdir)/../update-from-egg.sh || true
