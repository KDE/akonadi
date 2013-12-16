#! /bin/sh
$EXTRACTRC *.ui *.kcfg >> rc.cpp || exit 11
$XGETTEXT *.cpp *.h -o $podir/libakonadi-calendar.pot
rm -f rc.cpp

