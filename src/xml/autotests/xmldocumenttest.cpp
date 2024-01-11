/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "xmldocument.h"

#include <QObject>

#include <QTest>

using namespace Akonadi;

class XmlDocumentTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testDocumentLoad()
    {
        XmlDocument doc(QFINDTESTDATA("knutdemo.xml"), QFINDTESTDATA("../akonadi-xml.xsd"));
        QVERIFY(doc.isValid());
        QVERIFY(doc.lastError().isEmpty());
        QCOMPARE(doc.collections().count(), 9);

        Collection col = doc.collectionByRemoteId(QStringLiteral("c11"));
        QCOMPARE(col.name(), QStringLiteral("Inbox"));
        QCOMPARE(col.attributes().count(), 1);
        QCOMPARE(col.parentCollection().remoteId(), QStringLiteral("c1"));

        QCOMPARE(doc.childCollections(col).count(), 2);

        Item item = doc.itemByRemoteId(QStringLiteral("contact1"));
        QCOMPARE(item.mimeType(), QStringLiteral("text/directory"));
        QVERIFY(item.hasPayload());

        Item::List items = doc.items(col);
        QCOMPARE(items.count(), 1);
        item = items.first();
        QVERIFY(item.hasPayload());
        QCOMPARE(item.flags().count(), 1);
        QVERIFY(item.hasFlag("\\SEEN"));
    }
};

QTEST_MAIN(XmlDocumentTest)

#include "xmldocumenttest.moc"
