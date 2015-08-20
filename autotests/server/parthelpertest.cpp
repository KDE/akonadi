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

#include <aktest.h>
#include "entities.h"
#include "storage/parthelper.h"

#include <QObject>
#include <QtTest/QTest>
#include <QDebug>
#include <QDir>

#define QL1S(x) QString::fromLatin1(x)

using namespace Akonadi::Server;

class PartHelperTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testFileName()
    {
        akTestSetInstanceIdentifier(QString());

        Part p;
        p.setId(42);

        QString fileName = PartHelper::fileNameForPart(&p);
        QVERIFY(fileName.endsWith(QL1S("42")));
    }

    void testRemoveFile_data()
    {
        QTest::addColumn<QString>("instance");
        QTest::newRow("main") << QString();
        QTest::newRow("multi-instance") << QL1S("foo");
    }

    void testRemoveFile()
    {
        QFETCH(QString, instance);
        akTestSetInstanceIdentifier(instance);

        Part p;
        p.setId(23);
        const QString validFileName = PartHelper::storagePath() + QDir::separator() + PartHelper::fileNameForPart(&p);
        PartHelper::removeFile(validFileName);   // no throw
    }

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

    void testResolveAbsolutePath()
    {
#ifndef Q_OS_WIN
        QVERIFY(PartHelper::resolveAbsolutePath("foo").startsWith(QLatin1Char('/')));
        QCOMPARE(PartHelper::resolveAbsolutePath("/foo"), QString::fromLatin1("/foo"));
        QVERIFY(!PartHelper::resolveAbsolutePath("foo").contains(QL1S("//")));         // no double separator
#endif
    }
};

AKTEST_MAIN(PartHelperTest)

#include "parthelpertest.moc"
