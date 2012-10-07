#!/bin/bash

LIBLBFGS=$HOME/local
PKG=crfsuite-0.11
BINDIR=$HOME/build/$PKG
TARGET=`pwd`/$PKG-`/bin/arch`.tar.gz

rm -rf $BINDIR
./configure --prefix=$BINDIR --with-liblbfgs=$LIBLBFGS
make LDFLAGS=-all-static
make install
cd $BINDIR/..
tar cvzf $TARGET $PKG

