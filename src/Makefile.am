bin_PROGRAMS      = sparser
pkgdata_DATA		= conf/*.ini #conf/ru/*
#SUBDIRS           = 
EXTRA_DIST			= interfaces locale tests

sparser_SOURCES  = actions.c config_loader.c crc.c filters.c ini.c iniparser.c lexan.c \
	lexan_funcs.c msgloader.c msgmetadata.c msgparser.c msgviewer.c network.c \
	raw_message.c sparser.c stations.c utils.c
noinst_HEADERS   = actions.h common.h conf.h config_loader.h \
	crc.h errors.h filters.h ini.h iniparser.h lexan_funcs.h lexan.h \
	msgloader.h msgmetadata.h msgparser.h msgviewer.h network.h raw_message.h sparser.h \
	stations.h utils.h

localedir        = locale
DEFS             = -DLOCALE_DIR=\"$(localedir)\" @DEFS@

# Math.
sparser_LDADD		= -lm $(LIBINTL)
if WIN32
AM_CPPFLAGS       = -I$(top_builddir)/gnu_lib/lib -I$(top_srcdir)/gnu_lib/lib -I$(top_builddir)/intl -I$(top_srcdir)/intl
sparser_LDFLAGS   = -static-libgcc 
sparser_LDADD     += $(top_builddir)/gnu_lib/lib/libgnu.a -lwsock32 -lws2_32
endif
#AM_CPPFLAGS       = -O0 -g

## Additional targets.

common.h: errors.h conf.h
	touch common.h

all-local: version tags

clean-local:
	rm -f *~ core *.o

version:
	sed "s/\(BUILD_DATE\W\)\(".*"\)/\1\"`date '+%d.%m.%y %T'`\"/" sparser.h >.sparser.h
	mv -f .sparser.h sparser.h

.PHONY: version


##$(MAKE) $(AM_MAKEFLAGS)
