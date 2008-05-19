#!/bin/sh
if [ -L $0 ]; then
	abs_path=`readlink $0`
else
	abs_path=$0
fi

pushd `dirname $abs_path` > /dev/null

akonadidb=akonadi

cat create-unittest-values.sql | mysql -u root -D $akonadidb

popd > /dev/null
