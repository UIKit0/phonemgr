AM_CPPFLAGS = \
	-I$(top_srcdir)/intl \
	$(LIBGSM_CFLAGS)

EXTRA_DIST = \
	phonemgr-marshal.list

noinst_PROGRAMS = gsmtest glibgsmtest

noinst_LTLIBRARIES = libgsmwrap.la

libgsmwrap_la_SOURCES = \
	gsm.cc gsm.h phonemgr-listener.h phonemgr-listener-private.h \
	phonemgr-listener.cc \
	phonemgr-marshal.c phonemgr-marshal.h

gsmtest_SOURCES = \
	main.cc

gsmtest_LDADD = $(LIBGSM_LIBS) -lgsmme \
	libgsmwrap.la

glibgsmtest_SOURCES = \
	glibgsmtest.c

glibgsmtest_LDADD = $(LIBGSM_LIBS) -lgsmme \
	libgsmwrap.la

GENMARSHAL_GENERATED = phonemgr-marshal.c phonemgr-marshal.h

phonemgr-marshal.h: $(srcdir)/phonemgr-marshal.list
	( @GLIB_GENMARSHAL@ --prefix=phonemgr_marshal $(srcdir)/phonemgr-marshal.list --header > phonemgr-marshal.tmp \
	&& mv phonemgr-marshal.tmp phonemgr-marshal.h ) \
	|| ( rm -f phonemgr-marshal.tmp && exit 1 )

phonemgr-marshal.c: phonemgr-marshal.h
	( echo "#include \"phonemgr-marshal.h\"" >phonemgr-marshal.tmp \
	&& @GLIB_GENMARSHAL@ --prefix=phonemgr_marshal $(srcdir)/phonemgr-marshal.list --body >> phonemgr-marshal.tmp \
	&& mv phonemgr-marshal.tmp phonemgr-marshal.c ) \
	|| ( rm -f phonemgr-marshal.tmp && exit 1 )

BUILT_SOURCES = $(GENMARSHAL_GENERATED)

CLEANFILES = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)
