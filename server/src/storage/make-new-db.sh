#!/bin/sh
akonadidb=$HOME/.akonadi/akonadi.db

rm $akonadidb
cat create-database-schema.sql | sqlite3 $akonadidb
cat create-default-values.sql | sqlite3 $akonadidb
