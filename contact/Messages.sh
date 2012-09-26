#! /bin/sh
$EXTRACTRC *.ui actions/*.kcfg.cmake *.kcfg >> rc.cpp || exit 11
$XGETTEXT *.cpp *.h actions/*.cpp editor/*.cpp editor/im/*.cpp -o $podir/akonadicontact.pot
rm -f rc.cpp
