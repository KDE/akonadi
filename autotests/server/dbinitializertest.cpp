/*
    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>
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

#include "dbinitializertest.h"
#include "unittestschema.h"

#include <QtCore/QDebug>
#include <QtCore/QVariant>

#include "storage/dbinitializer.h"
#include <shared/aktest.h>

#define QL1S(x) QLatin1String(x)

using namespace Akonadi::Server;

Q_DECLARE_METATYPE(QVector<DbIntrospector::ForeignKey>)

class StatementCollector : public TestInterface
{
public:
    void execStatement(const QString &statement) Q_DECL_OVERRIDE
    {
        statements.push_back(statement);
    }

    QStringList statements;
};

class DbFakeIntrospector : public DbIntrospector
{
public:
    DbFakeIntrospector(const QSqlDatabase &database)
        : DbIntrospector(database)
        , m_hasTable(false)
        , m_hasIndex(false)
        , m_tableEmpty(true)
    {
    }

    bool hasTable(const QString &tableName) Q_DECL_OVERRIDE
    {
        Q_UNUSED(tableName);
        return m_hasTable;
    }
    bool hasIndex(const QString &tableName, const QString &indexName) Q_DECL_OVERRIDE
    {
        Q_UNUSED(tableName);
        Q_UNUSED(indexName);
        return m_hasIndex;
    }
    bool hasColumn(const QString &tableName, const QString &columnName) Q_DECL_OVERRIDE
    {
        Q_UNUSED(tableName);
        Q_UNUSED(columnName);
        return false;
    }
    bool isTableEmpty(const QString &tableName) Q_DECL_OVERRIDE
    {
        Q_UNUSED(tableName);
        return m_tableEmpty;
    }
    QVector< ForeignKey > foreignKeyConstraints(const QString &tableName) Q_DECL_OVERRIDE
    {
        Q_UNUSED(tableName);
        return m_foreignKeys;
    }

    QVector<ForeignKey> m_foreignKeys;
    bool m_hasTable;
    bool m_hasIndex;
    bool m_tableEmpty;
};

void DbInitializerTest::initTestCase()
{
    Q_INIT_RESOURCE(akonadidb);
}

void DbInitializerTest::testRun_data()
{
    QTest::addColumn<QString>("driverName");
    QTest::addColumn<QString>("filename");
    QTest::addColumn<bool>("hasTable");
    QTest::addColumn<QVector<DbIntrospector::ForeignKey> >("fks");
    QTest::addColumn<bool>("hasFks");

    QVector<DbIntrospector::ForeignKey> fks;

    QTest::newRow("mysql") << "QMYSQL" << ":dbinit_mysql" << false << fks << true;
    QTest::newRow("sqlite") << "QSQLITE" << ":dbinit_sqlite" << false << fks << false;
    QTest::newRow("psql") << "QPSQL" << ":dbinit_psql" << false << fks << true;

    DbIntrospector::ForeignKey fk;
    fk.name = QL1S("myForeignKeyIdentifier");
    fk.column = QL1S("collectionId");
    fk.refTable = QL1S("CollectionTable");
    fk.refColumn = QL1S("id");
    fk.onUpdate = QL1S("RESTRICT");
    fk.onDelete = QL1S("CASCADE");
    fks.push_back(fk);

    QTest::newRow("mysql (incremental)") << "QMYSQL" << ":dbinit_mysql_incremental" << true << fks << true;
    QTest::newRow("sqlite (incremental)") << "QSQLITE" << ":dbinit_sqlite_incremental" << true << fks << false;
    QTest::newRow("psql (incremental)") << "QPSQL" << ":dbinit_psql_incremental" << true << fks << true;
}

void DbInitializerTest::testRun()
{
    QFETCH(QString, driverName);
    QFETCH(QString, filename);
    QFETCH(bool, hasTable);
    QFETCH(QVector<DbIntrospector::ForeignKey>, fks);
    QFETCH(bool, hasFks);

    QFile file(filename);
    QVERIFY(file.open(QFile::ReadOnly));

    if (QSqlDatabase::drivers().contains(driverName)) {
        QSqlDatabase db = QSqlDatabase::addDatabase(driverName, driverName);
        UnitTestSchema schema;
        DbInitializer::Ptr initializer = DbInitializer::createInstance(db, &schema);
        QVERIFY(bool(initializer));

        StatementCollector collector;
        initializer->setTestInterface(&collector);
        DbFakeIntrospector *introspector = new DbFakeIntrospector(db);
        introspector->m_hasTable = hasTable;
        introspector->m_hasIndex = hasTable;
        introspector->m_tableEmpty = !hasTable;
        introspector->m_foreignKeys = fks;
        initializer->setIntrospector(DbIntrospector::Ptr(introspector));

        QVERIFY(initializer->run());
        QVERIFY(initializer->updateIndexesAndConstraints());
        QVERIFY(!collector.statements.isEmpty());

        Q_FOREACH (const QString &statement, collector.statements) {
            const QString expected = readNextStatement(&file).simplified();

            QString normalized = statement.simplified();
            normalized.replace(QLatin1String(" ,"), QLatin1String(","));
            normalized.replace(QLatin1String(" )"), QLatin1String(")"));
            QCOMPARE(normalized, expected);
        }

        QVERIFY(initializer->errorMsg().isEmpty());
        QCOMPARE(initializer->hasForeignKeyConstraints(), hasFks);
    }
}

QString DbInitializerTest::readNextStatement(QIODevice *io)
{
    QString statement;
    while (!io->atEnd()) {
        const QString line = QString::fromUtf8(io->readLine());
        if (line.trimmed().isEmpty() && !statement.isEmpty()) {
            return statement;
        }
        statement += line;
    }

    return statement;
}

AKTEST_MAIN(DbInitializerTest)
