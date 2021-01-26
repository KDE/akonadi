/*
    SPDX-FileCopyrightText: 2011 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

#include <aktest.h>
#include <private/standarddirs_p.h>
#include <shared/akranges.h>

#include <config-akonadi.h>
#include <storage/dbconfig.h>
#include <storage/dbconfigpostgresql.h>

#define QL1S(x) QStringLiteral(x)

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
    void testDbConfig()
    {
        // doesn't work, DbConfig has an internal singleton-like cache...
        // QFETCH( QString, driverName );
        const QString driverName(QL1S(AKONADI_DATABASE_BACKEND));

        // isolated config file to not conflict with a running instance
        akTestSetInstanceIdentifier(QL1S("unit-test"));

        {
            QSettings s(StandardDirs::serverConfigFile(StandardDirs::WriteOnly));
            s.setValue(QL1S("General/Driver"), driverName);
        }

        QScopedPointer<DbConfig> cfg(DbConfig::configuredDatabase());

        QVERIFY(!cfg.isNull());
        QCOMPARE(cfg->driverName(), driverName);
        QCOMPARE(cfg->databaseName(), QL1S("akonadi"));
        QCOMPARE(cfg->useInternalServer(), true);
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
