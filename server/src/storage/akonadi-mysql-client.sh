#! /bin/sh
# connect to mysqld started by akonadi
# useful for developing

akonadisocket=$HOME/.local/share/akonadi/db_misc/mysql.socket
mysql --socket=$akonadisocket akonadi
