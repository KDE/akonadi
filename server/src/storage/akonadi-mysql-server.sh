#! /bin/sh
# start mysqld as started by akonadi
# useful for developing
akonadihome=$HOME/.local/share/akonadi

/usr/sbin/mysqld \
	--datadir=$akonadihome/db_data/ \
	--log-bin=$akonadihome/db_log/ \
	--log-bin-index=$akonadihome/db_log/ \
	--socket=$akonadihome/db_misc/mysql.socket \
	--pid-file=$akonadihome/db_misc/mysql.pid \
	--skip-grant-tables \
	--skip-networking

