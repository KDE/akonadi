#!/bin/zsh
if [ -L $0 ]; then
	abs_path=`readlink $0`
else
	abs_path=$0
fi

pushd `dirname $abs_path` > /dev/null

akonadidb=akonadi
socketfile=$HOME/.akonadi/db_misc/mysql.socket
if ! [ -S $socketfile ]; then
	startserver=true
else
	unset startserver
fi

if ! [ -z "$startserver" ]; then
	/usr/sbin/mysqld --datadir=$HOME/.akonadi/db_data/ --socket=$socketfile --skip-grant-tables --skip-networking &
	sleep 2;
fi

cat create-unittest-values.sql | mysql --socket=$socketfile -D $akonadidb

if ! [ -z "$startserver" ]; then
	killall mysqld
fi

popd > /dev/null
