/*
    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "dbinitializertest.h"
#include "unittestschema.h"
#include <QIODevice>

#define DBINITIALIZER_UNITTEST
#include "storage/dbinitializer.cpp"
#undef DBINITIALIZER_UNITTEST

#include "shared/aktest.h"

#define QL1S(x) QLatin1StringView(x)

using namespace Akonadi::Server;

Q_DECLARE_METATYPE(QList<DbIntrospector::ForeignKey>)

class StatementCollector : public TestInterface
{
public:
    void execStatement(const QString &statement) override
    {
        statements.push_back(statement);
    }

    QStringList statements;
};

class DbFakeIntrospector : public DbIntrospector
{
public:
    explicit DbFakeIntrospector(const QSqlDatabase &database)
        : DbIntrospector(database)
        , m_hasTable(false)
        , m_hasIndex(false)
        , m_tableEmpty(true)
    {
    }

    bool hasTable(const QString &tableName) override
    {
        Q_UNUSED(tableName)
        return m_hasTable;
    }
    bool hasIndex(const QString &tableName, const QString &indexName) override
    {
        Q_UNUSED(tableName)
        Q_UNUSED(indexName)
        return m_hasIndex;
    }
    bool hasColumn(const QString &tableName, const QString &columnName) override
    {
        Q_UNUSED(tableName)
        Q_UNUSED(columnName)
        return false;
    }
    bool isTableEmpty(const QString &tableName) override
    {
        Q_UNUSED(tableName)
        return m_tableEmpty;
    }
    QList<ForeignKey> foreignKeyConstraints(const QString &tableName) override
    {
        Q_UNUSED(tableName)
        return m_foreignKeys;
    }

    QString getAutoIncrementValueQuery(const QString &tableName, const QString &columnName) override
    {
        Q_UNUSED(tableName);
        Q_UNUSED(columnName);
        return {};
    }

    QString updateAutoIncrementValueQuery(const QString &tableName, const QString &columnName, qint64 value) override
    {
        Q_UNUSED(tableName);
        Q_UNUSED(columnName);
        Q_UNUSED(value);

        return {};
    }

    QList<ForeignKey> m_foreignKeys;
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
    QTest::addColumn<QList<DbIntrospector::ForeignKey>>("fks");

    QList<DbIntrospector::ForeignKey> fks;

    QTest::newRow("mysql") << "QMYSQL"
                           << ":dbinit_mysql" << false << fks;
    QTest::newRow("sqlite") << "QSQLITE"
                            << ":dbinit_sqlite" << false << fks;
    QTest::newRow("psql") << "QPSQL"
                          << ":dbinit_psql" << false << fks;

    DbIntrospector::ForeignKey fk;
    fk.name = QL1S("myForeignKeyIdentifier");
    fk.column = QL1S("collectionId");
    fk.refTable = QL1S("CollectionTable");
    fk.refColumn = QL1S("id");
    fk.onUpdate = QL1S("RESTRICT");
    fk.onDelete = QL1S("CASCADE");
    fks.push_back(fk);

    QTest::newRow("mysql (incremental)") << "QMYSQL"
                                         << ":dbinit_mysql_incremental" << true << fks;
    QTest::newRow("sqlite (incremental)") << "QSQLITE"
                                          << ":dbinit_sqlite_incremental" << true << fks;
    QTest::newRow("psql (incremental)") << "QPSQL"
                                        << ":dbinit_psql_incremental" << true << fks;
}

void DbInitializerTest::testRun()
{
    QFETCH(QString, driverName);
    QFETCH(QString, filename);
    QFETCH(bool, hasTable);
    QFETCH(QList<DbIntrospector::ForeignKey>, fks);

    QFile file(filename);
    QVERIFY(file.open(QFile::ReadOnly));

    if (QSqlDatabase::drivers().contains(driverName)) {
        QSqlDatabase db = QSqlDatabase::addDatabase(driverName, driverName);
        UnitTestSchema schema;
        DbInitializer::Ptr initializer = DbInitializer::createInstance(db, &schema);
        QVERIFY(bool(initializer));

        StatementCollector collector;
        initializer->setTestInterface(&collector);
        auto introspector = new DbFakeIntrospector(db);
        introspector->m_hasTable = hasTable;
        introspector->m_hasIndex = hasTable;
        introspector->m_tableEmpty = !hasTable;
        introspector->m_foreignKeys = fks;
        initializer->setIntrospector(DbIntrospector::Ptr(introspector));

        QVERIFY(initializer->run());
        QVERIFY(initializer->updateIndexesAndConstraints());
        QVERIFY(!collector.statements.isEmpty());

        for (const QString &statement : std::as_const(collector.statements)) {
            const QString expected = readNextStatement(&file).simplified();

            QString normalized = statement.simplified();
            normalized.replace(QLatin1StringView(" ,"), QLatin1String(","));
            normalized.replace(QLatin1StringView(" )"), QLatin1String(")"));
            QCOMPARE(normalized, expected);
        }

        QVERIFY(initializer->errorMsg().isEmpty());
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

#include "moc_dbinitializertest.cpp"
