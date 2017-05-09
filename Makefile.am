AUTOMAKE_OPTIONS  = foreign 

#CPPLAGS          =
EXTRA_DIST        = m4/ChangeLog config.rpath mkinstalldirs valg_chk.sh src/gettext.h m4/Makefile.in prepare_src.sh checking iconv sparser.cmd
ACLOCAL_AMFLAGS   = -I m4
if WIN32
SUBDIRS           = gnu_lib/lib intl
ACLOCAL_AMFLAGS   += -I gnu_lib/m4
EXTRA_DIST        += gnu_lib/m4/Makefile.in gnu_lib/m4/gnulib-cache.m4
else
SUBDIRS           = intl
endif
SUBDIRS 				+= src docs po

full-archive:
	tar -C .. -czf ../sparser`echo $(VERSION)|sed 's/\.//g'`.tgz sparser

check-gettext:
	@if test x$(USE_NLS) != "xyes" ; then echo "Missing gettext. Rerun configure and check for" \
	"'checking whether to use NLS... yes'!" ; exit 1 ; fi

update-po: check-gettext
	@find $(srcdir)/src/ -name "*.c" -print | sort > $(srcdir)/po/POTFILES.in.2 ; \
	if diff $(srcdir)/po/POTFILES.in $(srcdir)/po/POTFILES.in.2 >/dev/null 2>&1 ; then \
		rm -f $(srcdir)/po/POTFILES.in.2 ; \
	else \
		mv $(srcdir)/po/POTFILES.in.2 $(srcdir)/po/POTFILES.in ; \
	fi
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-po

update-gmo: check-gettext
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-gmo
	cp $(top_builddir)/po/ru.gmo $(top_builddir)/src/locale/ru/LC_MESSAGES/sparser.mo

force-update-gmo: check-gettext
	touch po/*.po
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-gmo

force-update-gmo-%: check-gettext
	@language=`echo $@ | sed s/force-update-gmo-//` ; \
	if test ! -f po/$$language.po ; then echo "file po/$$language.po does not exist" ; exit 1 ; fi ; \
	touch po/$$language.po ; \
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-gmo

.PHONY: check-gettext update-po update-gmo force-update-gmo full-archive
