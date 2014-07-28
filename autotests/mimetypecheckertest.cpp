/*
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "mimetypecheckertest.h"
#include "testattribute.h"

#include "collection.h"
#include "item.h"

#include <kmimetype.h>

#include "krandom.h"

#include <qtest.h>

QTEST_MAIN( MimeTypeCheckerTest )

using namespace Akonadi;

MimeTypeCheckerTest::MimeTypeCheckerTest( QObject *parent )
  : QObject( parent )
{
  mCalendarSubTypes << QLatin1String("application/x-vnd.akonadi.calendar.event")
                    << QLatin1String("application/x-vnd.akonadi.calendar.todo");
}

void MimeTypeCheckerTest::initTestCase()
{
  QVERIFY( KMimeType::mimeType( QLatin1String("application/x-vnd.akonadi.calendar.event") ) );

  MimeTypeChecker emptyChecker;
  MimeTypeChecker calendarChecker;
  MimeTypeChecker subTypeChecker;
  MimeTypeChecker aliasChecker;

  // for testing reset through assignments
  const QLatin1String textPlain = QLatin1String( "text/plain" );
  mEmptyChecker.addWantedMimeType( textPlain );
  QVERIFY( !mEmptyChecker.wantedMimeTypes().isEmpty() );

  const QLatin1String textCalendar = QLatin1String( "text/calendar" );
  calendarChecker.addWantedMimeType( textCalendar );
  QCOMPARE( calendarChecker.wantedMimeTypes().count(), 1 );

  subTypeChecker.setWantedMimeTypes( mCalendarSubTypes );
  QCOMPARE( subTypeChecker.wantedMimeTypes().count(), 2 );

  const QLatin1String textVCard = QLatin1String( "text/directory" );
  aliasChecker.addWantedMimeType( textVCard );
  QCOMPARE( aliasChecker.wantedMimeTypes().count(), 1 );

  // test assignment works correctly
  mEmptyChecker    = emptyChecker;
  mCalendarChecker = calendarChecker;
  mSubTypeChecker  = subTypeChecker;
  mAliasChecker    = aliasChecker;

  QVERIFY( mEmptyChecker.wantedMimeTypes().isEmpty() );

  QCOMPARE( mCalendarChecker.wantedMimeTypes().count(), 1 );
  QCOMPARE( mCalendarChecker.wantedMimeTypes(), QStringList() << textCalendar );

  QCOMPARE( mSubTypeChecker.wantedMimeTypes().count(), 2 );
  const QSet<QString> calendarSubTypes = QSet<QString>::fromList( mCalendarSubTypes );
  const QSet<QString> wantedSubTypes = QSet<QString>::fromList( mSubTypeChecker.wantedMimeTypes() );
  QCOMPARE( wantedSubTypes, calendarSubTypes );

  QCOMPARE( mAliasChecker.wantedMimeTypes().count(), 1 );
  QCOMPARE( mAliasChecker.wantedMimeTypes(), QStringList() << textVCard );
}

void MimeTypeCheckerTest::testCollectionCheck()
{
  Collection invalidCollection;
  Collection emptyCollection( 1 );
  Collection calendarCollection( 2 );
  Collection eventCollection( 3 );
  Collection journalCollection( 4 );
  Collection vcardCollection( 5 );
  Collection aliasCollection( 6 );

  const QLatin1String textCalendar = QLatin1String( "text/calendar" );
  calendarCollection.setContentMimeTypes( QStringList() <<  textCalendar);
  const QLatin1String akonadiEvent = QLatin1String( "application/x-vnd.akonadi.calendar.event" );
  eventCollection.setContentMimeTypes( QStringList() << akonadiEvent );
  journalCollection.setContentMimeTypes( QStringList() << QLatin1String( "application/x-vnd.akonadi.calendar.journal" ) );
  const QLatin1String textDirectory = QLatin1String( "text/directory" );
  vcardCollection.setContentMimeTypes( QStringList() << textDirectory );
  aliasCollection.setContentMimeTypes( QStringList() << QLatin1String( "text/x-vcard" ) );

  Collection::List voidCollections;
  voidCollections << invalidCollection << emptyCollection;

  Collection::List subTypeCollections;
  subTypeCollections << eventCollection << journalCollection;

  Collection::List calendarCollections = subTypeCollections;
  calendarCollections << calendarCollection;

  Collection::List contactCollections;
  contactCollections << vcardCollection << aliasCollection;

  //// empty checker fails for all
  Collection::List collections = voidCollections + calendarCollections + contactCollections;
  foreach ( const Collection &collection, collections ) {
    QVERIFY( !mEmptyChecker.isWantedCollection( collection ) );
    QVERIFY( !MimeTypeChecker::isWantedCollection( collection, QString() ) );
  }

  //// calendar checker fails for void and contact collections
  collections = voidCollections + contactCollections;
  foreach ( const Collection &collection, collections ) {
    QVERIFY( !mCalendarChecker.isWantedCollection( collection ) );
    QVERIFY( !MimeTypeChecker::isWantedCollection( collection, textCalendar ) );
  }

  // but accepts all calendar collections
  collections = calendarCollections;
  foreach ( const Collection &collection, collections ) {
    QVERIFY( mCalendarChecker.isWantedCollection( collection ) );
    QVERIFY( MimeTypeChecker::isWantedCollection( collection, textCalendar ) );
  }

  //// sub type checker fails for all but the event collection
  collections = voidCollections + calendarCollections + contactCollections;
  collections.removeAll( eventCollection );
  foreach ( const Collection &collection, collections ) {
    QVERIFY( !mSubTypeChecker.isWantedCollection( collection ) );
    QVERIFY( !MimeTypeChecker::isWantedCollection( collection, akonadiEvent ) );
  }

  // but accepts the event collection
  collections = Collection::List() << eventCollection;
  foreach ( const Collection &collection, collections ) {
    QVERIFY( mSubTypeChecker.isWantedCollection( collection ) );
    QVERIFY( MimeTypeChecker::isWantedCollection( collection, akonadiEvent ) );
  }

  //// alias checker fails for void and calendar collections
  collections = voidCollections + calendarCollections;
  foreach ( const Collection &collection, collections ) {
    QVERIFY( !mAliasChecker.isWantedCollection( collection ) );
    QVERIFY( !MimeTypeChecker::isWantedCollection( collection, textDirectory ) );
  }

  // but accepts all contact collections
  collections = contactCollections;
  foreach ( const Collection &collection, collections ) {
    QVERIFY( mAliasChecker.isWantedCollection( collection ) );
    QVERIFY( MimeTypeChecker::isWantedCollection( collection, textDirectory ) );
  }
}

void MimeTypeCheckerTest::testItemCheck()
{
  Item invalidItem;
  Item emptyItem( 1 );
  Item calendarItem( 2 );
  Item eventItem( 3 );
  Item journalItem( 4 );
  Item vcardItem( 5 );
  Item aliasItem( 6 );

  const QLatin1String textCalendar = QLatin1String( "text/calendar" );
  calendarItem.setMimeType( textCalendar );
  const QLatin1String akonadiEvent = QLatin1String( "application/x-vnd.akonadi.calendar.event" );
  eventItem.setMimeType( akonadiEvent );
  journalItem.setMimeType( QLatin1String( "application/x-vnd.akonadi.calendar.journal" ) );
  const QLatin1String textDirectory = QLatin1String( "text/directory" );
  vcardItem.setMimeType( textDirectory );
  aliasItem.setMimeType( QLatin1String( "text/x-vcard" ) );

  Item::List voidItems;
  voidItems << invalidItem << emptyItem;

  Item::List subTypeItems;
  subTypeItems << eventItem << journalItem;

  Item::List calendarItems = subTypeItems;
  calendarItems << calendarItem;

  Item::List contactItems;
  contactItems << vcardItem << aliasItem;

  //// empty checker fails for all
  Item::List items = voidItems + calendarItems + contactItems;
  foreach ( const Item &item, items ) {
    QVERIFY( !mEmptyChecker.isWantedItem( item ) );
    QVERIFY( !MimeTypeChecker::isWantedItem( item, QString() ) );
  }

  //// calendar checker fails for void and contact items
  items = voidItems + contactItems;
  foreach ( const Item &item, items ) {
    QVERIFY( !mCalendarChecker.isWantedItem( item ) );
    QVERIFY( !MimeTypeChecker::isWantedItem( item, textCalendar ) );
  }

  // but accepts all calendar items
  items = calendarItems;
  foreach ( const Item &item, items ) {
    QVERIFY( mCalendarChecker.isWantedItem( item ) );
    QVERIFY( MimeTypeChecker::isWantedItem( item, textCalendar ) );
  }

  //// sub type checker fails for all but the event item
  items = voidItems + calendarItems + contactItems;
  items.removeAll( eventItem );
  foreach ( const Item &item, items ) {
    QVERIFY( !mSubTypeChecker.isWantedItem( item ) );
    QVERIFY( !MimeTypeChecker::isWantedItem( item, akonadiEvent ) );
  }

  // but accepts the event item
  items = Item::List() << eventItem;
  foreach ( const Item &item, items ) {
    QVERIFY( mSubTypeChecker.isWantedItem( item ) );
    QVERIFY( MimeTypeChecker::isWantedItem( item, akonadiEvent ) );
  }

  //// alias checker fails for void and calendar items
  items = voidItems + calendarItems;
  foreach ( const Item &item, items ) {
    QVERIFY( !mAliasChecker.isWantedItem( item ) );
    QVERIFY( !MimeTypeChecker::isWantedItem( item, textDirectory ) );
  }

  // but accepts all contact items
  items = contactItems;
  foreach ( const Item &item, items ) {
    QVERIFY( mAliasChecker.isWantedItem( item ) );
    QVERIFY( MimeTypeChecker::isWantedItem( item, textDirectory ) );
  }
}

void MimeTypeCheckerTest::testStringMatchEquivalent()
{
  // check that a random and thus not installed MIME type
  // can still be checked just like with direct string comparison

  const QLatin1String installedMimeType( "text/plain" );
  const QString randomMimeType = QLatin1String( "application/x-vnd.test." ) +
                                 KRandom::randomString( 10 );

  MimeTypeChecker installedTypeChecker;
  installedTypeChecker.addWantedMimeType( installedMimeType );

  MimeTypeChecker randomTypeChecker;
  randomTypeChecker.addWantedMimeType( randomMimeType );

  Item item1( 1 );
  item1.setMimeType( installedMimeType );
  Item item2( 2 );
  item2.setMimeType( randomMimeType );

  Collection collection1( 1 );
  collection1.setContentMimeTypes( QStringList() << installedMimeType );
  Collection collection2( 2 );
  collection2.setContentMimeTypes( QStringList() << randomMimeType );
  Collection collection3( 3 );
  collection3.setContentMimeTypes( QStringList() << installedMimeType << randomMimeType );

  QVERIFY( installedTypeChecker.isWantedItem( item1 ) );
  QVERIFY( !randomTypeChecker.isWantedItem( item1 ) );
  QVERIFY( MimeTypeChecker::isWantedItem( item1, installedMimeType ) );
  QVERIFY( !MimeTypeChecker::isWantedItem( item1, randomMimeType ) );

  QVERIFY( !installedTypeChecker.isWantedItem( item2 ) );
  QVERIFY( randomTypeChecker.isWantedItem( item2 ) );
  QVERIFY( !MimeTypeChecker::isWantedItem( item2, installedMimeType ) );
  QVERIFY( MimeTypeChecker::isWantedItem( item2, randomMimeType ) );

  QVERIFY( installedTypeChecker.isWantedCollection( collection1 ) );
  QVERIFY( !randomTypeChecker.isWantedCollection( collection1 ) );
  QVERIFY( MimeTypeChecker::isWantedCollection( collection1, installedMimeType ) );
  QVERIFY( !MimeTypeChecker::isWantedCollection( collection1, randomMimeType ) );

  QVERIFY( !installedTypeChecker.isWantedCollection( collection2 ) );
  QVERIFY( randomTypeChecker.isWantedCollection( collection2 ) );
  QVERIFY( !MimeTypeChecker::isWantedCollection( collection2, installedMimeType ) );
  QVERIFY( MimeTypeChecker::isWantedCollection( collection2, randomMimeType ) );

  QVERIFY( installedTypeChecker.isWantedCollection( collection3 ) );
  QVERIFY( randomTypeChecker.isWantedCollection( collection3 ) );
  QVERIFY( MimeTypeChecker::isWantedCollection( collection3, installedMimeType ) );
  QVERIFY( MimeTypeChecker::isWantedCollection( collection3, randomMimeType ) );
}

// kate: space-indent on; indent-width 2; replace-tabs on;
