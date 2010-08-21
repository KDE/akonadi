#! /bin/sh
$EXTRACTRC *.ui actions/*.kcfg *.kcfg >> rc.cpp || exit 11
$XGETTEXT *.cpp actions/*.cpp editor/*.cpp editor/im/*.cpp -o $podir/akonadicontact.pot
rm -f rc.cpp
