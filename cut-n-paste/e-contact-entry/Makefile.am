AM_CPPFLAGS =	\
	$(PHONEMGR_CFLAGS)

noinst_LTLIBRARIES = libecontact-entry.la

libecontact_entry_la_SOURCES =	\
	e-contact-entry.c	\
	e-contact-entry.h	\
	econtactentry-marshal.c

MARSHALFILES = econtactentry-marshal.c econtactentry-marshal.h
BUILT_SOURCES = $(MARSHALFILES)
econtactentry-marshal.c: econtactentry-marshal.h
	( $(GLIB_GENMARSHAL) --prefix=econtactentry_marshal $(srcdir)/econtactentry-marshal.list --header --body > econtactentry-marshal.c )
econtactentry-marshal.h: econtactentry-marshal.list
	( $(GLIB_GENMARSHAL) --prefix=econtactentry_marshal $(srcdir)/econtactentry-marshal.list --header > econtactentry-marshal.h )

EGGDIR=$(srcdir)/../../../contact-lookup-applet/src/
EGGFILES=e-contact-entry.c e-contact-entry.h
regenerate-built-sources:
	EGGFILES="$(EGGFILES)" EGGDIR="$(EGGDIR)" $(srcdir)/../update-from-egg.sh || true

CLEANFILES = $(BUILT_SOURCES)
