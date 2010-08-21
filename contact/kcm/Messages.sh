#! /bin/sh
$EXTRACTRC *.ui >> rc.cpp
$XGETTEXT *.cpp  -o $podir/kcm_akonadicontact_actions.pot
