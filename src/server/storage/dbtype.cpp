/*
    Copyright (c) 2012 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "dbtype.h"

using namespace Akonadi::Server;

DbType::Type DbType::type(const QSqlDatabase &db)
{
    return typeForDriverName(db.driverName());
}

DbType::Type DbType::typeForDriverName(const QString &driverName)
{
    if (driverName.startsWith(QLatin1String("QMYSQL"))) {
        return MySQL;
    }
    if (driverName == QLatin1String("QPSQL")) {
        return PostgreSQL;
    }
    if (driverName.startsWith(QLatin1String("QSQLITE"))) {
        return Sqlite;
    }
    return Unknown;
}

bool DbType::isSystemSQLite(const QSqlDatabase &db)
{
    return db.driverName() == QLatin1String("QSQLITE");
}
