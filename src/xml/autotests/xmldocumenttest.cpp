/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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
        XmlDocument doc(QStringLiteral(KDESRCDIR "/knutdemo.xml"));
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
