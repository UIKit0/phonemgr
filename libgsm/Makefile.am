
AM_CPPFLAGS =					\
	$(LIBGSM_CFLAGS)			\
	-DDATA_DIR="\"$(datadir)/$(PACKAGE)\""	\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"\
	$(WARN_CFLAGS)

EXTRA_DIST =				\
	phonemgr-marshal.list		\
	phones.xml

noinst_PROGRAMS = glibgsmtest test-sdp-search

noinst_LTLIBRARIES = libgsmwrap.la

libgsmwrap_la_SOURCES = \
	phonemgr-listener.h \
	phonemgr-listener.c \
	phonemgr-utils.c \
	phonemgr-utils.h \
	phonemgr-marshal.c phonemgr-marshal.h
libgsmwrap_la_LIBADD = $(LIBGSM_LIBS)

glibgsmtest_SOURCES = glibgsmtest.c
glibgsmtest_LDADD = libgsmwrap.la

test_sdp_search_SOURCES = test-sdp-search.c
test_sdp_search_LDADD = libgsmwrap.la

phones_DATA = phones.xml
phonesdir = $(datadir)/$(PACKAGE)

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

