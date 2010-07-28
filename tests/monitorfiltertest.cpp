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
#include <akonadi/private/notificationmessage_p.h>
#include <qtest_kde.h>
#include <QtTest>

using namespace Akonadi;

Q_DECLARE_METATYPE( Akonadi::NotificationMessage::Operation )
Q_DECLARE_METATYPE( Akonadi::NotificationMessage::Type )

class MonitorFilterTest : public QObject
{
  Q_OBJECT
  private Q_SLOTS:
    void filterConnected_data()
    {
      QTest::addColumn<Akonadi::NotificationMessage::Operation>( "op" );
      QTest::addColumn<QByteArray>( "signalName" );

      QTest::newRow( "itemAdded" ) << NotificationMessage::Add << QByteArray( SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)) );
      QTest::newRow( "itemChanged" ) << NotificationMessage::Modify << QByteArray( SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)) );
      QTest::newRow( "itemRemoved" ) << NotificationMessage::Remove << QByteArray( SIGNAL(itemRemoved(Akonadi::Item)) );
      QTest::newRow( "itemMoved" ) << NotificationMessage::Move << QByteArray( SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)) );
      QTest::newRow( "itemLinked" ) << NotificationMessage::Link << QByteArray( SIGNAL(itemLinked(Akonadi::Item,Akonadi::Collection)) );
      QTest::newRow( "itemUnlinked" ) << NotificationMessage::Unlink << QByteArray( SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection)) );
    }

    void filterConnected()
    {
      QFETCH( NotificationMessage::Operation, op );
      QFETCH( QByteArray, signalName );

      Monitor dummyMonitor;
      MonitorPrivate m( &dummyMonitor );

      NotificationMessage msg;
      msg.setUid( 1 );
      msg.setOperation( op );
      msg.setType( Akonadi::NotificationMessage::Item );

      QVERIFY( !m.acceptNotification( msg ) );
      m.monitorAll = true;
      QVERIFY( !m.acceptNotification( msg ) );
      QSignalSpy spy( &dummyMonitor, signalName );
      QVERIFY( spy.isValid() );
      QVERIFY( m.acceptNotification( msg ) );
      m.monitorAll = false;
      QVERIFY( !m.acceptNotification( msg ) );
    }

    void filterSession()
    {
      Monitor dummyMonitor;
      MonitorPrivate m( &dummyMonitor );
      m.monitorAll = true;
      QSignalSpy spy( &dummyMonitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)) );
      QVERIFY( spy.isValid() );

      NotificationMessage msg;
      msg.setUid( 1 );
      msg.setOperation( NotificationMessage::Add );
      msg.setType( Akonadi::NotificationMessage::Item );
      msg.setSessionId( "foo" );

      QVERIFY( m.acceptNotification( msg ) );
      m.sessions.append( "bar" );
      QVERIFY( m.acceptNotification( msg ) );
      m.sessions.append( "foo" );
      QVERIFY( !m.acceptNotification( msg ) );
    }

    void filterResource_data()
    {
      QTest::addColumn<Akonadi::NotificationMessage::Operation>( "op" );
      QTest::addColumn<Akonadi::NotificationMessage::Type>( "type" );
      QTest::addColumn<QByteArray>( "signalName" );

      QTest::newRow( "itemAdded" ) << NotificationMessage::Add << NotificationMessage::Item << QByteArray( SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)) );
      QTest::newRow( "itemChanged" ) << NotificationMessage::Modify << NotificationMessage::Item << QByteArray( SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)) );
      QTest::newRow( "itemRemoved" ) << NotificationMessage::Remove << NotificationMessage::Item << QByteArray( SIGNAL(itemRemoved(Akonadi::Item)) );
      QTest::newRow( "itemMoved" ) << NotificationMessage::Move << NotificationMessage::Item << QByteArray( SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)) );
      QTest::newRow( "itemLinked" ) << NotificationMessage::Link << NotificationMessage::Item << QByteArray( SIGNAL(itemLinked(Akonadi::Item,Akonadi::Collection)) );
      QTest::newRow( "itemUnlinked" ) << NotificationMessage::Unlink << NotificationMessage::Item << QByteArray( SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection)) );

      QTest::newRow( "colAdded" ) << NotificationMessage::Add << NotificationMessage::Collection << QByteArray( SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)) );
      QTest::newRow( "colChanged" ) << NotificationMessage::Modify << NotificationMessage::Collection << QByteArray( SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)) );
      QTest::newRow( "colRemoved" ) << NotificationMessage::Remove << NotificationMessage::Collection << QByteArray( SIGNAL(collectionRemoved(Akonadi::Collection)) );
      QTest::newRow( "colMoved" ) << NotificationMessage::Move << NotificationMessage::Collection << QByteArray( SIGNAL(collectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection)) );
    }

    void filterResource()
    {
      QFETCH( NotificationMessage::Operation, op );
      QFETCH( NotificationMessage::Type, type );
      QFETCH( QByteArray, signalName );

      Monitor dummyMonitor;
      MonitorPrivate m( &dummyMonitor );
      QSignalSpy spy( &dummyMonitor, signalName );
      QVERIFY( spy.isValid() );

      NotificationMessage msg;
      msg.setUid( 1 );
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
      QTest::addColumn<Akonadi::NotificationMessage::Operation>( "op" );
      QTest::addColumn<Akonadi::NotificationMessage::Type>( "type" );
      QTest::addColumn<QByteArray>( "signalName" );

      QTest::newRow( "itemMoved" ) << NotificationMessage::Move << NotificationMessage::Item << QByteArray( SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)) );
      QTest::newRow( "colMoved" ) << NotificationMessage::Move << NotificationMessage::Collection << QByteArray( SIGNAL(collectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection)) );
    }

    void filterDestinationResource()
    {
      QFETCH( NotificationMessage::Operation, op );
      QFETCH( NotificationMessage::Type, type );
      QFETCH( QByteArray, signalName );

      Monitor dummyMonitor;
      MonitorPrivate m( &dummyMonitor );
      QSignalSpy spy( &dummyMonitor, signalName );
      QVERIFY( spy.isValid() );

      NotificationMessage msg;
      msg.setOperation( op );
      msg.setType( type );
      msg.setResource( "foo" );
      msg.setItemParts( QSet<QByteArray>() << "bar" );
      msg.setSessionId( "mysession" );

      // using the right resource makes it pass
      QVERIFY( !m.acceptNotification( msg ) );
      m.resources.insert( "bla" );
      QVERIFY( !m.acceptNotification( msg ) );
      m.resources.insert( "bar" );
      QVERIFY( m.acceptNotification( msg ) );

      // filtering out the session overwrites the resource
      m.sessions.append( "mysession" );
      QVERIFY( !m.acceptNotification( msg ) );
    }
};

QTEST_KDEMAIN( MonitorFilterTest, NoGUI )

#include "monitorfiltertest.moc"
