## Process this file with automake to produce Makefile.in

SUBDIRS = po libgsm libegg src

EXTRA_DIST = \
	autogen.sh \
    intltool-extract.in intltool-merge.in intltool-update.in

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/ui/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(datadir)/$(PACKAGE)/ui/pixmaps; \
	  for pixmap in $(srcdir)/ui/pixmaps/*.png; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(datadir)/$(PACKAGE)/ui/pixmaps; \
	    fi \
	  done \
	fi
	if test -d $(srcdir)/ui; then \
	  $(mkinstalldirs) $(DESTDIR)$(datadir)/$(PACKAGE)/ui; \
	  for uifile in $(srcdir)/ui/*.glade $(srcdir)/ui/*.xml; do \
	    if test -f $$uifile; then \
	      $(INSTALL_DATA) $$uifile $(DESTDIR)$(datadir)/$(PACKAGE)/ui; \
	    fi \
	  done \
	fi

dist-hook:
	if test -d ui; then \
	  mkdir $(distdir)/ui; \
	  for uifile in ui/*.glade ui/*.xml; do \
	    if test -f $$uifile; then \
	      cp -p $$uifile $(distdir)/ui; \
	    fi \
	  done \
	fi
	if test -d ui/pixmaps; then \
	  mkdir $(distdir)/ui/pixmaps; \
	  for uifile in ui/pixmaps/*.png ui/pixmaps/*.xcf; do \
	    if test -f $$uifile; then \
	      cp -p $$uifile $(distdir)/ui/pixmaps; \
	    fi \
	  done \
	fi


