/*
    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
    if (driverName == QLatin1String("QSQLITE")) {
        return Sqlite;
    }
    return Unknown;
}
