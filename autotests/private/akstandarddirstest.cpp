/*
    SPDX-FileCopyrightText: 2011 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <aktest.h>
#include <private/standarddirs_p.h>

#include <QObject>
#include <QTest>

#define QL1S(x) QStringLiteral(x)

using namespace Akonadi;

class AkStandardDirsTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testCondigFile()
    {
        akTestSetInstanceIdentifier(QString());
        QVERIFY(StandardDirs::agentsConfigFile(StandardDirs::ReadOnly).endsWith(QL1S("agentsrc")));
        QVERIFY(StandardDirs::agentsConfigFile(StandardDirs::ReadWrite).endsWith(QL1S("agentsrc")));
        QVERIFY(!StandardDirs::agentsConfigFile(StandardDirs::ReadWrite).endsWith(QL1S("foo/agentsrc")));

        akTestSetInstanceIdentifier(QL1S("foo"));
        QVERIFY(StandardDirs::agentsConfigFile(StandardDirs::ReadOnly).endsWith(QL1S("agentsrc")));
        QVERIFY(StandardDirs::agentsConfigFile(StandardDirs::ReadWrite).endsWith(QL1S("instance/foo/agentsrc")));
    }

    void testSaveDir()
    {
        akTestSetInstanceIdentifier(QString());
        QVERIFY(StandardDirs::saveDir("data").endsWith(QL1S("/akonadi")));
        QVERIFY(!StandardDirs::saveDir("data").endsWith(QL1S("foo/akonadi")));

        akTestSetInstanceIdentifier(QL1S("foo"));
        QVERIFY(StandardDirs::saveDir("data").endsWith(QL1S("/akonadi/instance/foo")));
    }
};

AKTEST_MAIN(AkStandardDirsTest)

#include "akstandarddirstest.moc"
