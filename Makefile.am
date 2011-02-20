## Process this file with automake to produce Makefile.in

SUBDIRS = po libgsm cut-n-paste src gnome-bluetooth telepathy data

EXTRA_DIST = \
	autogen.sh \
	intltool-extract.in intltool-merge.in intltool-update.in

DISTCLEANFILES = intltool-extract intltool-merge intltool-update

# Build ChangeLog from GIT history
ChangeLog:
	@if test -f $(top_srcdir)/.git/HEAD; then \
		git log --stat V_0_65.. > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog
