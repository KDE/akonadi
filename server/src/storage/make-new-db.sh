#!/bin/sh
rm akonadi.db
cat create-database-schema.sql | sqlite3 akonadi.db
cat create-default-values.sql | sqlite3 akonadi.db

