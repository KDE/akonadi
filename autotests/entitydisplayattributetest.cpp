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

#include "entitydisplayattribute.h"

#include <QtCore/QObject>

#include <qtest.h>

using namespace Akonadi;

class EntityDisplayAttributeTest : public QObject
{
  Q_OBJECT
  private Q_SLOTS:
    void testDeserialize_data()
    {
      QTest::addColumn<QByteArray>( "input" );
      QTest::addColumn<QString>( "name" );
      QTest::addColumn<QString>( "icon" );
      QTest::addColumn<QString>( "activeIcon");
      QTest::addColumn<QByteArray>( "output" );

      QTest::newRow( "empty" ) << QByteArray("(\"\" \"\")") << QString() << QString() << QString() << QByteArray("(\"\" \"\" \"\" ())");
      QTest::newRow( "name+icon" ) << QByteArray( "(\"name\" \"icon\")") << QStringLiteral( "name" ) << QStringLiteral( "icon" ) << QString()<<QByteArray( "(\"name\" \"icon\" \"\" ())" );
      QTest::newRow("name+icon+activeIcon") << QByteArray( "(\"name\" \"icon\" \"activeIcon\")") << QStringLiteral( "name" ) << QStringLiteral( "icon" ) << QStringLiteral("activeIcon") << QByteArray( "(\"name\" \"icon\" \"activeIcon\" ())" );
    }

    void testDeserialize()
    {
      QFETCH( QByteArray, input );
      QFETCH( QString, name );
      QFETCH( QString, icon );
      QFETCH( QString, activeIcon);
      QFETCH( QByteArray, output );

      EntityDisplayAttribute* attr = new EntityDisplayAttribute();
      attr->deserialize( input );
      QCOMPARE( attr->displayName(), name );
      QCOMPARE( attr->iconName(), icon );
      QCOMPARE( attr->activeIconName(), activeIcon );

      QCOMPARE( attr->serialized(), output );

      EntityDisplayAttribute *copy = attr->clone();
      QCOMPARE( copy->serialized(), output );

      delete attr;
      delete copy;
    }
};

QTEST_MAIN( EntityDisplayAttributeTest )

#include "entitydisplayattributetest.moc"

