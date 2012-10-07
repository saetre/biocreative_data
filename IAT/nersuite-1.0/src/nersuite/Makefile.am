# $Id:$

bin_PROGRAMS = nersuite_main

#man_MANS = nersuite.main.1

#EXTRA_DIST = ${man_MANS}

#EXTRA_DIST =

nersuite_main_SOURCES = \
	nersuite.cpp \
	nersuite.h \
	typedefs.h \
	optparse/optparse.h \
	optparse/optparse.nersuite.h \
	string_utils/string_utils.h \
	string_utils/string_utils.cpp \
	text_loader/text_loader.h \
	text_loader/text_loader.cpp \
	FExtor/FExtor.h \
	FExtor/FExtor.cpp

nersuite_main_CPPFLAGS = @CFLAGS@
INCLUDES = -I./optparse -I./string_utils -I./text_loader -I./FExtor @INCLUDES@

#AM_CFLAGS = @CFLAGS@
#INCLUDES = @INCLUDES@
#AM_LDFLAGS = @LDFLAGS@
#nersuite_CFLAGS = -I$(top_builddir)/lib/crf/include
#nersuite_LDADD = $(top_builddir)/lib/crf/libcrf.la

