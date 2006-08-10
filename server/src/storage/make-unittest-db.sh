#!/bin/sh
akonadidb=$HOME/.akonadi/akonadi.db

cat create-unittest-values.sql | sqlite3 $akonadidb
