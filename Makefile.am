## Process this file with automake to produce Makefile.in

SUBDIRS = po libgsm libegg src

EXTRA_DIST = \
	autogen.sh \
	intltool-extract.in intltool-merge.in intltool-update.in

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/ui; then \
	  $(mkinstalldirs) $(DESTDIR)$(datadir)/$(PACKAGE); \
	  for uifile in $(srcdir)/ui/*.glade $(srcdir)/ui/*.png; do \
	    if test -f $$uifile; then \
	      $(INSTALL_DATA) $$uifile $(DESTDIR)$(datadir)/$(PACKAGE)/; \
	    fi \
	  done \
	fi

dist-hook:
	if test -d ui; then \
	  mkdir $(distdir)/ui; \
	  for uifile in ui/*.glade ui/*.png; do \
	    if test -f $$uifile; then \
	      cp -p $$uifile $(distdir)/ui/; \
	    fi \
	  done \
	fi

