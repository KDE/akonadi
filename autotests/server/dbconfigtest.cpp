/*
    SPDX-FileCopyrightText: 2011 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "aktest.h"
#include "config-akonadi.h"

#include "private/standarddirs_p.h"
#include "shared/akranges.h"
#include "storage/dbconfig.h"
#include "storage/dbconfigpostgresql.h"

#include <QScopeGuard>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QtEnvironmentVariables>

using namespace Akonadi;
using namespace Akonadi::Server;
using namespace AkRanges;

class TestableDbConfigPostgresql : public DbConfigPostgresql
{
public:
    QStringList postgresSearchPaths(const QTemporaryDir &dir)
    {
        return DbConfigPostgresql::postgresSearchPaths(dir.path());
    }
};

class DbConfigTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testDbConfig_data()
    {
        QTest::addColumn<QString>("driverName");
        QTest::addColumn<QString>("dbName");
        QTest::addColumn<bool>("useInternalServer");

        QStandardPaths::setTestModeEnabled(true);
        akTestSetInstanceIdentifier(QStringLiteral("unit-test"));

        const QString sqlitedb = StandardDirs::saveDir("data") + QStringLiteral("/akonadi.db");
        QTest::newRow("QSQLITE") << QStringLiteral("QSQLITE") << sqlitedb << false;
        QTest::newRow("QMYSQL") << QStringLiteral("QMYSQL") << QStringLiteral("akonadi") << true;
        QTest::newRow("QPSQL") << QStringLiteral("QPSQL") << QStringLiteral("akonadi_unit_test") << true;
    }
    void testDbConfig()
    {
        QFETCH(QString, driverName);
        QFETCH(QString, dbName);
        QFETCH(bool, useInternalServer);

        // isolated config file to not conflict with a running instance
        QStandardPaths::setTestModeEnabled(true);
        akTestSetInstanceIdentifier(QStringLiteral("unit-test"));

        {
            QSettings s(StandardDirs::serverConfigFile(StandardDirs::ReadWrite), QSettings::IniFormat);
            s.setValue(QStringLiteral("General/Driver"), driverName);
        }

        const auto destroyDbConfig = qScopeGuard([] {
            DbConfig::destroy();
        });
        auto *cfg = DbConfig::configuredDatabase();

        QVERIFY(cfg != nullptr);

        QCOMPARE(cfg->driverName(), driverName);
        QCOMPARE(cfg->databaseName(), dbName);
        QCOMPARE(cfg->useInternalServer(), useInternalServer);
        QCOMPARE(cfg->sizeThreshold(), 4096LL);
    }

    void testPostgresVersionedLookup()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QStringList versions{QStringLiteral("10.2"),
                                   QStringLiteral("10.0"),
                                   QStringLiteral("9.5"),
                                   QStringLiteral("12.4"),
                                   QStringLiteral("8.0"),
                                   QStringLiteral("12.0")};
        for (const auto &version : versions) {
            QVERIFY(QDir(dir.path()).mkdir(version));
        }

        TestableDbConfigPostgresql dbConfig;
        const auto paths = dbConfig.postgresSearchPaths(dir) | Views::filter([&dir](const auto &path) {
                               return path.startsWith(dir.path());
                           })
            | Views::transform([&dir](const auto &path) {
                               return QString(path).remove(dir.path() + QStringLiteral("/")).remove(QStringLiteral("/bin"));
                           })
            | Actions::toQList;

        const QStringList expected{QStringLiteral("12.4"),
                                   QStringLiteral("12.0"),
                                   QStringLiteral("10.2"),
                                   QStringLiteral("10.0"),
                                   QStringLiteral("9.5"),
                                   QStringLiteral("8.0")};
        QCOMPARE(paths, expected);
    }
};

AKTEST_MAIN(DbConfigTest)

#include "dbconfigtest.moc"
