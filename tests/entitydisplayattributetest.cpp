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

#include <akonadi/entitydisplayattribute.h>

#include <QtCore/QObject>

#include <qtest_kde.h>

using namespace Akonadi;

class EntityDisplayAttributeTest : public QObject
{
  Q_OBJECT
  private slots:
    void testDeserialize_data()
    {
      QTest::addColumn<QByteArray>( "input" );
      QTest::addColumn<QString>( "name" );
      QTest::addColumn<QString>( "icon" );
      QTest::addColumn<bool>( "hide" );
      QTest::addColumn<QByteArray>( "output" );

      QTest::newRow( "empty" ) << QByteArray("(\"\" \"\")") << QString() << QString() << false << QByteArray("(\"\" \"\" false)");
      QTest::newRow( "name+icon" ) << QByteArray( "(\"name\" \"icon\")") << QString( "name" ) << QString( "icon" ) << false << QByteArray( "(\"name\" \"icon\" false)" );
      QTest::newRow( "name+icon+visible" ) << QByteArray( "(\"name\" \"icon\" false)") << QString( "name" ) << QString( "icon" ) << false << QByteArray( "(\"name\" \"icon\" false)" );
      QTest::newRow( "name+icon+hidden" ) << QByteArray( "(\"name\" \"icon\" true)") << QString( "name" ) << QString( "icon" ) << true << QByteArray( "(\"name\" \"icon\" true)" );
    }

    void testDeserialize()
    {
      QFETCH( QByteArray, input );
      QFETCH( QString, name );
      QFETCH( QString, icon );
      QFETCH( bool, hide );
      QFETCH( QByteArray, output );

      EntityDisplayAttribute* attr = new EntityDisplayAttribute();
      attr->deserialize( input );
      QCOMPARE( attr->displayName(), name );
      QCOMPARE( attr->iconName(), icon );
      QCOMPARE( attr->isHidden(), hide );

      QCOMPARE( attr->serialized(), output );

      EntityDisplayAttribute *copy = attr->clone();
      QCOMPARE( copy->serialized(), output );

      delete attr;
      delete copy;
    }
};

QTEST_KDEMAIN( EntityDisplayAttributeTest, NoGUI )

#include "entitydisplayattributetest.moc"

