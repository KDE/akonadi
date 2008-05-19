#!/bin/sh
if [ -L $0 ]; then
	abs_path=`readlink $0`
else
	abs_path=$0
fi

pushd `dirname $abs_path` > /dev/null

akonadidb=$HOME/.akonadi/akonadi.db

cat create-unittest-values.sql | sqlite3 $akonadidb

popd > /dev/null
