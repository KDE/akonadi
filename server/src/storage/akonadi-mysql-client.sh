#! /bin/sh
# connect to mysqld started by akonadi
# useful for developing

akonadisocket="$HOME/.local/share/akonadi/socket-`hostname`/mysql.socket"
mysql --socket=$akonadisocket akonadi
