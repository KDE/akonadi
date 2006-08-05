#!/bin/sh
akonadidb=$HOME/.akonadi/akonadi.db

cat create-default-values.sql | sqlite3 $akonadidb
