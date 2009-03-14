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

#include <akonadi/xml/xmldocument.h>

#include <QtCore/QObject>

#include <qtest_kde.h>

using namespace Akonadi;

class XmlDocumentTest : public QObject
{
  Q_OBJECT
  private slots:
    void testDocumentLoad()
    {
      XmlDocument doc( KDESRCDIR "/knutdemo.xml" );
      QVERIFY( doc.isValid() );
      QVERIFY( doc.lastError().isEmpty() );
      QCOMPARE( doc.collections().count(), 9 );

      Collection col = doc.collectionByRemoteId( "c11" );
      QCOMPARE( col.name(), QString( "Inbox" ) );
      QCOMPARE( col.attributes().count(), 1 );
      QCOMPARE( col.parentRemoteId(), QString("c1") );

      Item item = doc.itemByRemoteId( "contact1" );
      QCOMPARE( item.mimeType(), QString( "text/directory" ) );
      QVERIFY( item.hasPayload() );

      Item::List items = doc.items( col );
      QCOMPARE( items.count(), 1 );
      item = items.first();
      QVERIFY( item.hasPayload() );
      QCOMPARE( item.flags().count(), 1 );
      QVERIFY( item.hasFlag( "\\Seen" ) );
    }
};

QTEST_KDEMAIN( XmlDocumentTest, NoGUI )

#include "xmldocumenttest.moc"
