#! /bin/sh
$EXTRACTRC `find . src -name "*.ui"` >> rc.cpp
$XGETTEXT `find . src -name "*.cpp" -o -name "*.h"` -o $podir/libakonadi5.pot
