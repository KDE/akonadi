#! /bin/sh
# connect to mysqld started by akonadi
# useful for developing

if [ -z "$1" ]; then
  akonadisocket="$HOME/.local/share/akonadi/socket-`hostname`/mysql.socket"
else
  if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "Usage: $0 [instance identifier]"
    exit 1;
  fi
  akonadisocket="$HOME/.local/share/akonadi/instance/$1/socket-`hostname`/mysql.socket"
fi

mysql --socket=$akonadisocket akonadi
