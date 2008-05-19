#! /bin/sh
# start mysqld as started by akonadi
# useful for developing

akonadihome=$HOME/.local/share/akonadi
globalconfig=$KDEDIR/share/akonadi/mysql-global.conf
localconfig=$HOME/.config/akonadi/mysql-local.conf
if [ -f $globalconfig ]; then
	cat $globalconfig $localconfig > $akonadihome/mysql.conf
fi

/usr/sbin/mysqld \
	--defaults-file=$akonadihome/mysql.conf \
	--datadir=$akonadihome/db_data/ \
	--socket=$akonadihome/db_misc/mysql.socket

