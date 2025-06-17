/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "../collectionutils.h"
#include "collection.h"
#include "qtest_akonadi.h"

using namespace Akonadi;

class CollectionUtilsTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testHasValidHierarchicalRID_data()
    {
        QTest::addColumn<Collection>("collection");
        QTest::addColumn<bool>("isHRID");

        QTest::newRow("empty") << Collection() << false;
        QTest::newRow("root") << Collection::root() << true;
        Collection c;
        c.setParentCollection(Collection::root());
        QTest::newRow("one level not ok") << c << false;
        c.setRemoteId(QStringLiteral("r1"));
        QTest::newRow("one level ok") << c << true;
        Collection c2;
        c2.setParentCollection(c);
        QTest::newRow("two level not ok") << c2 << false;
        c2.setRemoteId(QStringLiteral("r2"));
        QTest::newRow("two level ok") << c2 << true;
        c2.parentCollection().setRemoteId(QString());
        QTest::newRow("mid RID missing") << c2 << false;
    }

    void testHasValidHierarchicalRID()
    {
        QFETCH(Collection, collection);
        QFETCH(bool, isHRID);
        QCOMPARE(CollectionUtils::hasValidHierarchicalRID(collection), isHRID);
    }

    void testPersistentParentCollection()
    {
        Collection col1(1);
        Collection col2(2);
        Collection col3(3);

        col2.setParentCollection(col3);
        col1.setParentCollection(col2);

        Collection assigned = col1;
        QCOMPARE(assigned.parentCollection(), col2);
        QCOMPARE(assigned.parentCollection().parentCollection(), col3);

        Collection copied(col1);
        QCOMPARE(copied.parentCollection(), col2);
        QCOMPARE(copied.parentCollection().parentCollection(), col3);
    }
};

QTEST_AKONADI_CORE_MAIN(CollectionUtilsTest)

#include "collectionutilstest.moc"
