
AM_CPPFLAGS = \
	$(TELEPATHY_CFLAGS)				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DDATA_DIR="\"$(pkgdatadir)\""			\
	-DVERSION="\"$(VERSION)\""			\
	-DPACKAGE="\"$(PACKAGE)\""			\
	-I$(srcdir)/../libgsm				\
	-I$(builddir)/../libgsm

if HAVE_TELEPATHY
libexec_PROGRAMS = telepathy-sms

telepathy_sms_SOURCES =			\
	connection.c			\
	connection.h			\
	connection-manager.c		\
	connection-manager.h		\
	debug.c				\
	debug.h				\
	im-channel.c			\
	im-channel-factory.c		\
	im-channel-factory.h		\
	im-channel.h			\
	main.c

telepathy_sms_LDADD =			\
	$(TELEPATHY_LIBS)		\
	../libgsm/libgsmwrap.la

endif

