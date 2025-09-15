#! /bin/sh
if test -z "`which xsltproc`"; then
	echo "No xlstproc found!"
	exit 1;
fi

case $1 in
create)
	xsltproc --stringparam code header entities.xsl akonadidb.xml > entities.h
	xsltproc --stringparam code source entities.xsl akonadidb.xml > entities.cpp
	xsltproc entities-dox.xsl akonadidb.xml > Database.dox
;;
cleanup)
	rm -f entities.h entities.cpp
	rm -f Database.dox
;;
esac
