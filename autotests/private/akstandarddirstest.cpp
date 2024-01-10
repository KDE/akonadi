/*
    SPDX-FileCopyrightText: 2011 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <aktest.h>
#include <private/standarddirs_p.h>

#include <QObject>
#include <QTest>

using namespace Akonadi;

class AkStandardDirsTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testCondigFile()
    {
        akTestSetInstanceIdentifier(QString());
        QVERIFY(StandardDirs::agentsConfigFile(StandardDirs::ReadOnly).endsWith(QStringLiteral("agentsrc")));
        QVERIFY(StandardDirs::agentsConfigFile(StandardDirs::ReadWrite).endsWith(QStringLiteral("agentsrc")));
        QVERIFY(!StandardDirs::agentsConfigFile(StandardDirs::ReadWrite).endsWith(QStringLiteral("foo/agentsrc")));

        akTestSetInstanceIdentifier(QStringLiteral("foo"));
        QVERIFY(StandardDirs::agentsConfigFile(StandardDirs::ReadOnly).endsWith(QStringLiteral("agentsrc")));
        QVERIFY(StandardDirs::agentsConfigFile(StandardDirs::ReadWrite).endsWith(QStringLiteral("instance/foo/agentsrc")));
    }

    void testSaveDir()
    {
        akTestSetInstanceIdentifier(QString());
#ifdef Q_OS_WIN // See buildFullRelPath() in standarddirs.cpp
        QVERIFY(StandardDirs::saveDir("data").endsWith(QStringLiteral("/akonadi/data")));
#else
        QVERIFY(StandardDirs::saveDir("data").endsWith(QStringLiteral("/akonadi")));
#endif
        QVERIFY(!StandardDirs::saveDir("data").endsWith(QStringLiteral("foo/akonadi")));

        akTestSetInstanceIdentifier(QStringLiteral("foo"));
#ifdef Q_OS_WIN // See buildFullRelPath() in standarddirs.cpp
        QVERIFY(StandardDirs::saveDir("data").endsWith(QStringLiteral("/akonadi/data/instance/foo")));
#else
        QVERIFY(StandardDirs::saveDir("data").endsWith(QStringLiteral("/akonadi/instance/foo")));
#endif
    }
};

AKTEST_MAIN(AkStandardDirsTest)

#include "akstandarddirstest.moc"
