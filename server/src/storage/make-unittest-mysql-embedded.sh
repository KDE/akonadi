#!/bin/sh
if [ -L $0 ]; then
	abs_path=`readlink $0`
else
	abs_path=$0
fi

pushd `dirname $abs_path` > /dev/null

akonadidb=akonadi

/usr/sbin/mysqld --datadir=$HOME/.akonadi/db/ --socket=$HOME/.akonadi/mysql.socket --skip-grant-tables --skip-networking &
sleep 2;

cat create-unittest-values.sql | mysql --socket=$HOME/.akonadi/mysql.socket -D $akonadidb

killall mysqld

popd > /dev/null
