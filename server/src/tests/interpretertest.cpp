/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "interpretertest.h"

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( InterpreterTest )

using namespace Akonadi;

void InterpreterTest::testSimple()
{
  SearchParser parser;

  SearchInterpreterItem *item = parser.parse( "(name = 'hello world')" );

  QVERIFY( item != 0 );
  QVERIFY( item->isLeafItem() );
  QCOMPARE( item->key(), QLatin1String( "name" ) );
  QCOMPARE( item->comparator(), QLatin1String( "=" ) );
  QCOMPARE( item->pattern(), QLatin1String( "hello world" ) );

  delete item;
}

void InterpreterTest::testNested()
{
  SearchParser parser;

  SearchInterpreterItem *item = parser.parse( "(| (name = 'foo') (money != 'bar'))" );
  QVERIFY( item != 0 );
  QVERIFY( !item->isLeafItem() );

  QList<SearchInterpreterItem*> childItems = item->childItems();
  QCOMPARE( childItems.count(), 2 );

  SearchInterpreterItem *childItem = childItems[ 0 ];
  QVERIFY( childItem->isLeafItem() );
  QCOMPARE( childItem->key(), QLatin1String( "name" ) );
  QCOMPARE( childItem->comparator(), QLatin1String( "=" ) );
  QCOMPARE( childItem->pattern(), QLatin1String( "foo" ) );

  childItem = childItems[ 1 ];
  QVERIFY( childItem->isLeafItem() );
  QCOMPARE( childItem->key(), QLatin1String( "money" ) );
  QCOMPARE( childItem->comparator(), QLatin1String( "!=" ) );
  QCOMPARE( childItem->pattern(), QLatin1String( "bar" ) );

  delete item;
}

void InterpreterTest::testDoubleNested()
{
  SearchParser parser;

  SearchInterpreterItem *item = parser.parse( "(| (& (name = 'foo') (me != 'you') (he != 'she')) (money != 'bar'))" );
  QVERIFY( item != 0 );
  QVERIFY( !item->isLeafItem() );

  QList<SearchInterpreterItem*> childItems = item->childItems();
  QCOMPARE( childItems.count(), 2 );

  SearchInterpreterItem *childItem = childItems[ 0 ];
  QVERIFY( !childItem->isLeafItem() );

  QList<SearchInterpreterItem*> subChildItems = childItem->childItems();
  QCOMPARE( subChildItems.count(), 3 );

  SearchInterpreterItem *subChildItem = subChildItems[ 0 ];
  QVERIFY( subChildItem->isLeafItem() );
  QCOMPARE( subChildItem->key(), QLatin1String( "name" ) );
  QCOMPARE( subChildItem->comparator(), QLatin1String( "=" ) );
  QCOMPARE( subChildItem->pattern(), QLatin1String( "foo" ) );

  subChildItem = subChildItems[ 1 ];
  QVERIFY( subChildItem->isLeafItem() );
  QCOMPARE( subChildItem->key(), QLatin1String( "me" ) );
  QCOMPARE( subChildItem->comparator(), QLatin1String( "!=" ) );
  QCOMPARE( subChildItem->pattern(), QLatin1String( "you" ) );

  subChildItem = subChildItems[ 2 ];
  QVERIFY( subChildItem->isLeafItem() );
  QCOMPARE( subChildItem->key(), QLatin1String( "he" ) );
  QCOMPARE( subChildItem->comparator(), QLatin1String( "!=" ) );
  QCOMPARE( subChildItem->pattern(), QLatin1String( "she" ) );

  childItem = childItems[ 1 ];
  QVERIFY( childItem->isLeafItem() );
  QCOMPARE( childItem->key(), QLatin1String( "money" ) );
  QCOMPARE( childItem->comparator(), QLatin1String( "!=" ) );
  QCOMPARE( childItem->pattern(), QLatin1String( "bar" ) );

  delete item;
}

#include "interpretertest.moc"
