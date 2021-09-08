#! /bin/sh
$EXTRACTRC `find -name "*.ui"` >> rc.cpp
$XGETTEXT `find -name "*.cpp" -o -name "*.h" | grep -v '/tests/' | grep -v '/autotests/'` -o $podir/libakonadi5.pot
