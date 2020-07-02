/*
    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <aktest.h>
#include "entities.h"
#include "storage/parthelper.h"

#include <QObject>
#include <QTest>
#include <QDir>

#define QL1S(x) QString::fromLatin1(x)

using namespace Akonadi::Server;

class PartHelperTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
#if 0
    void testFileName()
    {
        akTestSetInstanceIdentifier(QString());

        Part p;
        p.setId(42);

        QString fileName = PartHelper::fileNameForPart(&p);
        QVERIFY(fileName.endsWith(QL1S("42")));
    }
#endif

    void testRemoveFile_data()
    {
        QTest::addColumn<QString>("instance");
        QTest::newRow("main") << QString();
        QTest::newRow("multi-instance") << QL1S("foo");
    }

#if 0
    void testRemoveFile()
    {
        QFETCH(QString, instance);
        akTestSetInstanceIdentifier(instance);

        Part p;
        p.setId(23);
        const QString validFileName = PartHelper::storagePath() + QDir::separator() + PartHelper::fileNameForPart(&p);
        PartHelper::removeFile(validFileName);   // no throw
    }
#endif

#if 0
    void testInvalidRemoveFile_data()
    {
        QTest::addColumn<QString>("fileName");
        QTest::newRow("empty") << QString();
        QTest::newRow("relative") << QL1S("foo");
        QTest::newRow("absolute") << QL1S("/foo");

        akTestSetInstanceIdentifier(QL1S("foo"));
        Part p;
        p.setId(23);
        QTest::newRow("wrong instance") << PartHelper::fileNameForPart(&p);
    }
#endif

#if 0
    void testInvalidRemoveFile()
    {
        QFETCH(QString, fileName);
        akTestSetInstanceIdentifier(QString());
        try {
            PartHelper::removeFile(fileName);
        } catch (const PartHelperException &e) {
            return; // all good
        }
        QVERIFY(false);   // didn't throw
    }
#endif

#if 0
    void testStorageLocation()
    {
        akTestSetInstanceIdentifier(QString());
        const QString mainLocation = PartHelper::storagePath();
        QVERIFY(mainLocation.endsWith(QDir::separator()));
        QVERIFY(mainLocation.startsWith(QDir::separator()));

        akTestSetInstanceIdentifier(QL1S("foo"));
        QVERIFY(PartHelper::storagePath().endsWith(QDir::separator()));
        QVERIFY(PartHelper::storagePath().startsWith(QDir::separator()));
        QVERIFY(mainLocation != PartHelper::storagePath());
    }
#endif

#if 0
    void testResolveAbsolutePath()
    {
#ifndef Q_OS_WIN
        QVERIFY(PartHelper::resolveAbsolutePath("foo").startsWith(QLatin1Char('/')));
        QCOMPARE(PartHelper::resolveAbsolutePath("/foo"), QString::fromLatin1("/foo"));
        QVERIFY(!PartHelper::resolveAbsolutePath("foo").contains(QL1S("//")));         // no double separator
#endif
    }
#endif
};

AKTEST_MAIN(PartHelperTest)

#include "parthelpertest.moc"
