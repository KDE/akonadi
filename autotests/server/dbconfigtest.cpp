/*
    Copyright (c) 2011 Volker Krause <vkrause@kde.org>

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
#include <QSettings>

#include <aktest.h>
#include <private/standarddirs_p.h>

#include <storage/dbconfig.h>
#include <config-akonadi.h>

#define QL1S(x) QStringLiteral(x)

using namespace Akonadi;
using namespace Akonadi::Server;

class DbConfigTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testDbConfig()
    {
        // doesn't work, DbConfig has an internal singleton-like cache...
        //QFETCH( QString, driverName );
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
        QCOMPARE(cfg->sizeThreshold(), 4096ll);
    }
};

AKTEST_MAIN(DbConfigTest)

#include "dbconfigtest.moc"
