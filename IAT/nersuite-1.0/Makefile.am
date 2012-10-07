# $Id: Makefile.am 87 2008-11-11 12:55:16Z naoaki $

SUBDIRS = src/tokenizer src/gtagger src/nersuite

docdir = $(prefix)/share/doc/@PACKAGE@
doc_DATA = README INSTALL COPYING AUTHORS ChangeLog

#models_gtaggerdir = $(prefix)/models_gtagger
#models_gtagger_DATA = \
#pkgdata_DATA = \
#	models_gtagger/WORDNETLICENSE \
#	....

#models_nersuitedir = $(prefix)/models_nersuite
#models_nersuite_DATA = 
	
EXTRA_DIST = \
	sample/test.txt
#	${models_gtagger_DATA} \
#	${models_nersuite_DATA} \
#	sample/test.sh

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4
