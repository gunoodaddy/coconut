#!/bin/bash

ReConfAndBuild=$1

if [ "${ReConfAndBuild}" = "yes" ]; then
	echo "All clean & reconfiguration..."
	autoconf
	make maintainer-clean
	configure
	make
fi

if [ ! -f Makefile ]; then
	echo Makefile not found
	exit 1
fi

VERSION=`make print-version`
if [ "${VERSION}" = "" ]; then
	echo invalid version..
	exit 1
fi

cd ..

DISTDIR="coconut-${VERSION}"

if [ -d "${DISTDIR}" ]; then
	rm -rf "${DISTDIR}"
fi

cp -rf coconut "${DISTDIR}"

cd "${DISTDIR}"
automake
autoconf
configure
make maintainer-clean
cd ..

tar czvf "${DISTDIR}".tar.gz "${DISTDIR}"
