# $Id:$

bin_PROGRAMS = nersuite_gtagger

#man_MANS = nersuite.gtagger.1

#EXTRA_DIST = ${man_MANS}

#EXTRA_DIST =

nersuite_gtagger_SOURCES = \
	run.gtagger.cpp \
	geniatagger-3.0.1/bidir.cpp \
	geniatagger-3.0.1/chunking.cpp \
	geniatagger-3.0.1/maxent.cpp \
	geniatagger-3.0.1/maxent.h \
	geniatagger-3.0.1/morph.cpp \
	geniatagger-3.0.1/tokenize.cpp \
	geniatagger-3.0.1/postag.cpp \
	geniatagger-3.0.1/common.h

nersuite_gtagger_CPPFLAGS = -Wno-deprecated @CFLAGS@
INCLUDES = -I./geniatagger-3.0.1

#AM_CFLAGS = @CFLAGS@
#INCLUDES = @INCLUDES@
#AM_LDFLAGS = @LDFLAGS@



