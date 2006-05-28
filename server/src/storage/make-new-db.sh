#!/bin/sh
if [ -L $0 ]; then
	abs_path=`readlink $0`
else
	abs_path=$0
fi

pushd `dirname $abs_path` > /dev/null

rm akonadi.db
cat create-database-schema.sql | sqlite3 akonadi.db
cat create-default-values.sql | sqlite3 akonadi.db

popd > /dev/null
