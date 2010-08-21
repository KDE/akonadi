#! /bin/sh
$EXTRACTRC *.ui >> rc.cpp
$XGETTEXT *.cpp *.h conflicthandling/*.cpp -o $podir/libakonadi.pot
