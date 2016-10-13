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

#include <QObject>
#include <QTest>

#include <aktest.h>
#include <storage/dbtype.h>

#define QL1S(x) QLatin1String(x)

using namespace Akonadi::Server;

Q_DECLARE_METATYPE(DbType::Type)

class DbTypeTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testDriverName_data()
    {
        QTest::addColumn<QString>("driverName");
        QTest::addColumn<DbType::Type>("dbType");

        QTest::newRow("mysql") << "QMYSQL" << DbType::MySQL;
        QTest::newRow("sqlite") << "QSQLITE" << DbType::Sqlite;
        QTest::newRow("sqlite3") << "QSQLITE3" << DbType::Sqlite;
        QTest::newRow("psql") << "QPSQL" << DbType::PostgreSQL;
    }

    void testDriverName()
    {
        QFETCH(QString, driverName);
        QFETCH(DbType::Type, dbType);

        QCOMPARE(DbType::typeForDriverName(driverName), dbType);

        if (QSqlDatabase::drivers().contains(driverName)) {
            QSqlDatabase db = QSqlDatabase::addDatabase(driverName, driverName);
            QCOMPARE(DbType::type(db), dbType);
        }
    }
};

AKTEST_MAIN(DbTypeTest)

#include "dbtypetest.moc"
