/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#include <QtCore/QObject>

#include "monitor_p.h"
#include <private/notificationmessagev3_p.h>
#include <qtest.h>
#include <qtest_akonadi.h>

using namespace Akonadi;

Q_DECLARE_METATYPE( Akonadi::NotificationMessageV2::Operation )
Q_DECLARE_METATYPE( QSet<QByteArray> )

class MonitorFilterTest : public QObject
{
  Q_OBJECT
  private Q_SLOTS:
    void initTestCase()
    {
      AkonadiTest::checkTestIsIsolated();
      qRegisterMetaType<Akonadi::Item>();
      qRegisterMetaType<Akonadi::Collection>();
      qRegisterMetaType<QSet<QByteArray> >();
      qRegisterMetaType<Akonadi::Item::List>();
    }

    void filterConnected_data()
    {
      QTest::addColumn<Akonadi::NotificationMessageV2::Operation>( "op" );
      QTest::addColumn<QByteArray>( "signalName" );

      QTest::newRow( "itemAdded" ) << NotificationMessageV2::Add << QByteArray( SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)) );
      QTest::newRow( "itemChanged" ) << NotificationMessageV2::Modify << QByteArray( SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)) );
      QTest::newRow( "itemsFlagsChanged" ) << NotificationMessageV2::ModifyFlags << QByteArray( SIGNAL(itemsFlagsChanged(Akonadi::Item::List,QSet<QByteArray>,QSet<QByteArray>)) );
      QTest::newRow( "itemRemoved" ) << NotificationMessageV2::Remove << QByteArray( SIGNAL(itemRemoved(Akonadi::Item)) );
      QTest::newRow( "itemMoved" ) << NotificationMessageV2::Move << QByteArray( SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)) );
      QTest::newRow( "itemLinked" ) << NotificationMessageV2::Link << QByteArray( SIGNAL(itemLinked(Akonadi::Item,Akonadi::Collection)) );
      QTest::newRow( "itemUnlinked" ) << NotificationMessageV2::Unlink << QByteArray( SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection)) );
    }

    void filterConnected()
    {
      QFETCH( NotificationMessageV2::Operation, op );
      QFETCH( QByteArray, signalName );

      Monitor dummyMonitor;
      MonitorPrivate m( 0, &dummyMonitor );

      NotificationMessageV3 msg;
      msg.addEntity( 1 );
      msg.setOperation( op );
      msg.setType( Akonadi::NotificationMessageV2::Items );

      QVERIFY( !m.acceptNotification( msg ) );
      m.monitorAll = true;
      QVERIFY( m.acceptNotification( msg ) );
      QSignalSpy spy( &dummyMonitor, signalName );
      QVERIFY( spy.isValid() );
      QVERIFY( m.acceptNotification( msg ) );
      m.monitorAll = false;
      QVERIFY( !m.acceptNotification( msg ) );
    }

    void filterSession()
    {
      Monitor dummyMonitor;
      MonitorPrivate m( 0, &dummyMonitor );
      m.monitorAll = true;
      QSignalSpy spy( &dummyMonitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)) );
      QVERIFY( spy.isValid() );

      NotificationMessageV3 msg;
      msg.addEntity( 1 );
      msg.setOperation( NotificationMessageV2::Add );
      msg.setType( Akonadi::NotificationMessageV2::Items );
      msg.setSessionId( "foo" );

      QVERIFY( m.acceptNotification( msg ) );
      m.sessions.append( "bar" );
      QVERIFY( m.acceptNotification( msg ) );
      m.sessions.append( "foo" );
      QVERIFY( !m.acceptNotification( msg ) );
    }

    void filterResource_data()
    {
      QTest::addColumn<Akonadi::NotificationMessageV2::Operation>( "op" );
      QTest::addColumn<Akonadi::NotificationMessageV2::Type>( "type" );
      QTest::addColumn<QByteArray>( "signalName" );

      QTest::newRow( "itemAdded" ) << NotificationMessageV2::Add << NotificationMessageV2::Items << QByteArray( SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)) );
      QTest::newRow( "itemChanged" ) << NotificationMessageV2::Modify << NotificationMessageV2::Items << QByteArray( SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)) );
      QTest::newRow( "itemsFlagsChanged" ) << NotificationMessageV2::ModifyFlags << NotificationMessageV2::Items << QByteArray( SIGNAL(itemsFlagsChanged(Akonadi::Item::List,QSet<QByteArray>,QSet<QByteArray>)) );
      QTest::newRow( "itemRemoved" ) << NotificationMessageV2::Remove << NotificationMessageV2::Items << QByteArray( SIGNAL(itemRemoved(Akonadi::Item)) );
      QTest::newRow( "itemMoved" ) << NotificationMessageV2::Move << NotificationMessageV2::Items << QByteArray( SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)) );
      QTest::newRow( "itemLinked" ) << NotificationMessageV2::Link << NotificationMessageV2::Items << QByteArray( SIGNAL(itemLinked(Akonadi::Item,Akonadi::Collection)) );
      QTest::newRow( "itemUnlinked" ) << NotificationMessageV2::Unlink << NotificationMessageV2::Items << QByteArray( SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection)) );

      QTest::newRow( "colAdded" ) << NotificationMessageV2::Add << NotificationMessageV2::Collections << QByteArray( SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)) );
      QTest::newRow( "colChanged" ) << NotificationMessageV2::Modify << NotificationMessageV2::Collections << QByteArray( SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)) );
      QTest::newRow( "colRemoved" ) << NotificationMessageV2::Remove << NotificationMessageV2::Collections << QByteArray( SIGNAL(collectionRemoved(Akonadi::Collection)) );
      QTest::newRow( "colMoved" ) << NotificationMessageV2::Move << NotificationMessageV2::Collections << QByteArray( SIGNAL(collectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection)) );
      QTest::newRow( "colSubscribed" ) << NotificationMessageV2::Subscribe << NotificationMessageV2::Collections << QByteArray( SIGNAL(collectionSubscribed(Akonadi::Collection,Akonadi::Collection)) );
      QTest::newRow( "colSubscribed" ) << NotificationMessageV2::Unsubscribe << NotificationMessageV2::Collections << QByteArray( SIGNAL(collectionUnsubscribed(Akonadi::Collection)) );
    }

    void filterResource()
    {
      QFETCH( NotificationMessageV2::Operation, op );
      QFETCH( NotificationMessageV2::Type, type );
      QFETCH( QByteArray, signalName );

      Monitor dummyMonitor;
      MonitorPrivate m( 0, &dummyMonitor );
      QSignalSpy spy( &dummyMonitor, signalName );
      QVERIFY( spy.isValid() );

      NotificationMessageV3 msg;
      msg.addEntity( 1 );
      msg.setOperation( op );
      msg.setParentCollection( 2 );
      msg.setType( type );
      msg.setResource( "foo" );
      msg.setSessionId( "mysession" );

      // using the right resource makes it pass
      QVERIFY( !m.acceptNotification( msg ) );
      m.resources.insert( "bar" );
      QVERIFY( !m.acceptNotification( msg ) );
      m.resources.insert( "foo" );
      QVERIFY( m.acceptNotification( msg ) );

      // filtering out the session overwrites the resource
      m.sessions.append( "mysession" );
      QVERIFY( !m.acceptNotification( msg ) );
    }

    void filterDestinationResource_data()
    {
      QTest::addColumn<Akonadi::NotificationMessageV2::Operation>( "op" );
      QTest::addColumn<Akonadi::NotificationMessageV2::Type>( "type" );
      QTest::addColumn<QByteArray>( "signalName" );

      QTest::newRow( "itemMoved" ) << NotificationMessageV2::Move << NotificationMessageV2::Items << QByteArray( SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)) );
      QTest::newRow( "colMoved" ) << NotificationMessageV2::Move << NotificationMessageV2::Collections << QByteArray( SIGNAL(collectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection)) );
    }

    void filterDestinationResource()
    {
      QFETCH( NotificationMessageV2::Operation, op );
      QFETCH( NotificationMessageV2::Type, type );
      QFETCH( QByteArray, signalName );

      Monitor dummyMonitor;
      MonitorPrivate m( 0, &dummyMonitor );
      QSignalSpy spy( &dummyMonitor, signalName );
      QVERIFY( spy.isValid() );

      NotificationMessageV3 msg;
      msg.setOperation( op );
      msg.setType( type );
      msg.setResource( "foo" );
      msg.setDestinationResource( "bar" );
      msg.setSessionId( "mysession" );
      msg.addEntity( 1 );

      // using the right resource makes it pass
      QVERIFY( !m.acceptNotification( msg ) );
      m.resources.insert( "bla" );
      QVERIFY( !m.acceptNotification( msg ) );
      m.resources.insert( "bar" );
      QVERIFY( m.acceptNotification( msg ) );

      // filtering out the mimetype does not overwrite resources
      msg.addEntity( 1, QString(), QString(), "my/type" );
      QVERIFY( m.acceptNotification( msg ) );

      // filtering out the session overwrites the resource
      m.sessions.append( "mysession" );
      QVERIFY( !m.acceptNotification( msg ) );
    }

    void filterMimeType_data()
    {
      QTest::addColumn<Akonadi::NotificationMessageV2::Operation>( "op" );
      QTest::addColumn<Akonadi::NotificationMessageV2::Type>( "type" );
      QTest::addColumn<QByteArray>( "signalName" );

      QTest::newRow( "itemAdded" ) << NotificationMessageV2::Add << NotificationMessageV2::Items << QByteArray( SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)) );
      QTest::newRow( "itemChanged" ) << NotificationMessageV2::Modify << NotificationMessageV2::Items << QByteArray( SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)) );
      QTest::newRow( "itemsFlagsChanged" ) << NotificationMessageV2::ModifyFlags << NotificationMessageV2::Items << QByteArray( SIGNAL(itemsFlagsChanged(Akonadi::Item::List,QSet<QByteArray>,QSet<QByteArray>)) );
      QTest::newRow( "itemRemoved" ) << NotificationMessageV2::Remove << NotificationMessageV2::Items << QByteArray( SIGNAL(itemRemoved(Akonadi::Item)) );
      QTest::newRow( "itemMoved" ) << NotificationMessageV2::Move << NotificationMessageV2::Items << QByteArray( SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)) );
      QTest::newRow( "itemLinked" ) << NotificationMessageV2::Link << NotificationMessageV2::Items << QByteArray( SIGNAL(itemLinked(Akonadi::Item,Akonadi::Collection)) );
      QTest::newRow( "itemUnlinked" ) << NotificationMessageV2::Unlink << NotificationMessageV2::Items << QByteArray( SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection)) );

      QTest::newRow( "colAdded" ) << NotificationMessageV2::Add << NotificationMessageV2::Collections << QByteArray( SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)) );
      QTest::newRow( "colChanged" ) << NotificationMessageV2::Modify << NotificationMessageV2::Collections << QByteArray( SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)) );
      QTest::newRow( "colRemoved" ) << NotificationMessageV2::Remove << NotificationMessageV2::Collections << QByteArray( SIGNAL(collectionRemoved(Akonadi::Collection)) );
      QTest::newRow( "colMoved" ) << NotificationMessageV2::Move << NotificationMessageV2::Collections << QByteArray( SIGNAL(collectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection)) );
      QTest::newRow( "colSubscribed" ) << NotificationMessageV2::Subscribe << NotificationMessageV2::Collections << QByteArray( SIGNAL(collectionSubscribed(Akonadi::Collection,Akonadi::Collection)) );
      QTest::newRow( "colSubscribed" ) << NotificationMessageV2::Unsubscribe << NotificationMessageV2::Collections << QByteArray( SIGNAL(collectionUnsubscribed(Akonadi::Collection)) );
    }

    void filterMimeType()
    {
      QFETCH( NotificationMessageV2::Operation, op );
      QFETCH( NotificationMessageV2::Type, type );
      QFETCH( QByteArray, signalName );

      Monitor dummyMonitor;
      MonitorPrivate m( 0, &dummyMonitor );
      QSignalSpy spy( &dummyMonitor, signalName );
      QVERIFY( spy.isValid() );

      NotificationMessageV3 msg;
      msg.addEntity( 1, QString(), QString(), "my/type" );
      msg.setOperation( op );
      msg.setParentCollection( 2 );
      msg.setType( type );
      msg.setResource( "foo" );
      msg.setSessionId( "mysession" );

      // using the right resource makes it pass
      QVERIFY( !m.acceptNotification( msg ) );
      m.mimetypes.insert( "your/type" );
      QVERIFY( !m.acceptNotification( msg ) );
      m.mimetypes.insert( "my/type" );
      QCOMPARE( m.acceptNotification( msg ), type == NotificationMessageV2::Items );

      // filter out the resource does not overwrite mimetype
      m.resources.insert( "bar" );
      QCOMPARE( m.acceptNotification( msg ), type == NotificationMessageV2::Items );

      // filtering out the session overwrites the mimetype
      m.sessions.append( "mysession" );
      QVERIFY( !m.acceptNotification( msg ) );
    }

    void filterCollection_data()
    {
      QTest::addColumn<Akonadi::NotificationMessageV2::Operation>( "op" );
      QTest::addColumn<Akonadi::NotificationMessageV2::Type>( "type" );
      QTest::addColumn<QByteArray>( "signalName" );

      QTest::newRow( "itemAdded" ) << NotificationMessageV2::Add << NotificationMessageV2::Items << QByteArray( SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)) );
      QTest::newRow( "itemChanged" ) << NotificationMessageV2::Modify << NotificationMessageV2::Items << QByteArray( SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)) );
      QTest::newRow( "itemsFlagsChanged" ) << NotificationMessageV2::ModifyFlags << NotificationMessageV2::Items << QByteArray( SIGNAL(itemsFlagsChanged(Akonadi::Item::List,QSet<QByteArray>,QSet<QByteArray>)) );
      QTest::newRow( "itemRemoved" ) << NotificationMessageV2::Remove << NotificationMessageV2::Items << QByteArray( SIGNAL(itemRemoved(Akonadi::Item)) );
      QTest::newRow( "itemMoved" ) << NotificationMessageV2::Move << NotificationMessageV2::Items << QByteArray( SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)) );
      QTest::newRow( "itemLinked" ) << NotificationMessageV2::Link << NotificationMessageV2::Items << QByteArray( SIGNAL(itemLinked(Akonadi::Item,Akonadi::Collection)) );
      QTest::newRow( "itemUnlinked" ) << NotificationMessageV2::Unlink << NotificationMessageV2::Items << QByteArray( SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection)) );

      QTest::newRow( "colAdded" ) << NotificationMessageV2::Add << NotificationMessageV2::Collections << QByteArray( SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)) );
      QTest::newRow( "colChanged" ) << NotificationMessageV2::Modify << NotificationMessageV2::Collections << QByteArray( SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)) );
      QTest::newRow( "colRemoved" ) << NotificationMessageV2::Remove << NotificationMessageV2::Collections << QByteArray( SIGNAL(collectionRemoved(Akonadi::Collection)) );
      QTest::newRow( "colMoved" ) << NotificationMessageV2::Move << NotificationMessageV2::Collections << QByteArray( SIGNAL(collectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection)) );
      QTest::newRow( "colSubscribed" ) << NotificationMessageV2::Subscribe << NotificationMessageV2::Collections << QByteArray( SIGNAL(collectionSubscribed(Akonadi::Collection,Akonadi::Collection)) );
      QTest::newRow( "colSubscribed" ) << NotificationMessageV2::Unsubscribe << NotificationMessageV2::Collections << QByteArray( SIGNAL(collectionUnsubscribed(Akonadi::Collection)) );
    }

    void filterCollection()
    {
      QFETCH( NotificationMessageV2::Operation, op );
      QFETCH( NotificationMessageV2::Type, type );
      QFETCH( QByteArray, signalName );

      Monitor dummyMonitor;
      MonitorPrivate m( 0, &dummyMonitor );
      QSignalSpy spy( &dummyMonitor, signalName );
      QVERIFY( spy.isValid() );

      NotificationMessageV3 msg;
      msg.addEntity( 1, QString(), QString(), "my/type" );
      msg.setOperation( op );
      msg.setParentCollection( 2 );
      msg.setType( type );
      msg.setResource( "foo" );
      msg.setSessionId( "mysession" );

      // using the right resource makes it pass
      QVERIFY( !m.acceptNotification( msg ) );
      m.collections.append( Collection( 3 ) );
      QVERIFY( !m.acceptNotification( msg ) );

      for ( int colId = 0; colId < 3; ++colId ) { // 0 == root, 1 == this, 2 == parent
        if ( colId == 1 && type == NotificationMessageV2::Items )
          continue;

        m.collections.clear();
        m.collections.append( Collection( colId ) );

        QVERIFY( m.acceptNotification( msg ) );

        // filter out the resource does overwrite collection
        m.resources.insert( "bar" );
        QVERIFY( !m.acceptNotification( msg ) );
        m.resources.clear();

        // filter out the mimetype does overwrite collection, for item operations (mimetype filter has no effect on collections)
        m.mimetypes.insert( "your/type" );
        QCOMPARE( !m.acceptNotification( msg ), type == NotificationMessageV2::Items );
        m.mimetypes.clear();

        // filtering out the session overwrites the mimetype
        m.sessions.append( "mysession" );
        QVERIFY( !m.acceptNotification( msg ) );
        m.sessions.clear();

        // filter non-matching resource and matching mimetype make it pass
        m.resources.insert( "bar" );
        m.mimetypes.insert( "my/type" );
        QVERIFY( m.acceptNotification( msg ) );
        m.resources.clear();
        m.mimetypes.clear();
      }
    }
};

QTEST_MAIN( MonitorFilterTest )

#include "monitorfiltertest.moc"
