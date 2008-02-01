#!/bin/sh
if [ -L $0 ]; then
	abs_path=`readlink $0`
else
	abs_path=$0
fi

pushd `dirname $abs_path` > /dev/null

akonadidb=akonadi
socketfile=$HOME/.local/share/akonadi/db_misc/mysql.socket
logfile=$HOME/.local/share/akonadi/db_data/mysql-bin.index
if ! [ -S $socketfile ]; then
	startserver=true
else
	unset startserver
fi

if ! [ -z "$startserver" ]; then
  akonadihome=$HOME/.local/share/akonadi
  globalconfig=$KDEDIR/share/akonadi/mysql-global.conf
  localconfig=$HOME/.config/akonadi/mysql-local.conf
  if [ -f $globalconfig ]; then
	  cat $globalconfig $localconfig > $akonadihome/mysql.conf
  fi

  /usr/sbin/mysqld \
	  --defaults-file=$akonadihome/mysql.conf \
  	--datadir=$akonadihome/db_data/ \
	  --socket=$akonadihome/db_misc/mysql.socket \
    --skip-grant-tables \
    --skip-networking &

	sleep 2;
fi

cat create-unittest-values.sql | mysql --socket=$socketfile -D $akonadidb

if ! [ -z "$startserver" ]; then
	killall mysqld
fi

popd > /dev/null
