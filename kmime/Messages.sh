#! /bin/sh
$EXTRACTRC *.kcfg >> rc.cpp
$XGETTEXT *.cpp *.h -o $podir/libakonadi-kmime.pot
