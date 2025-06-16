#! /bin/sh
$EXTRACTRC `find -name "*.ui"` >> rc.cpp
$XGETTEXT `find -name "*.cpp" -o -name \*.qml -o -name "*.h" | grep -v '/server/dbmigrator/' | grep -v '/tests/' | grep -v '/autotests/'` -o $podir/libakonadi6.pot
