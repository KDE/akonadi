#! /bin/sh
$EXTRACTRC actions/*.kcfg >> rc.cpp || exit 11
$XGETTEXT *.cpp actions/*.cpp editor/*.cpp -o $podir/akonadicontact.pot
rm -f rc.cpp
