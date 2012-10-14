/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include <akonadi/changerecorder.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/agentmanager.h>

#include <QtCore/QObject>
#include <QtCore/QSettings>

#include <qtest_akonadi.h>

using namespace Akonadi;

Q_DECLARE_METATYPE(QSet<QByteArray>)

class ChangeRecorderTest : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase()
    {
      qRegisterMetaType<Akonadi::Item>();
      qRegisterMetaType<QSet<QByteArray> >();
      AkonadiTest::checkTestIsIsolated();
      AkonadiTest::setAllResourcesOffline();

      settings = new QSettings( "kde.org", "akonadi-changerecordertest", this );
    }

    // After each test
    void cleanup()
    {
      settings->clear();
    }

    void testChangeRecorder()
    {
      ChangeRecorder *rec = createChangeRecorder();
      QVERIFY( rec->isEmpty() );

      QSignalSpy spy( rec, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)) );
      QVERIFY( spy.isValid() );
      QSignalSpy cspy( rec, SIGNAL(changesAdded()) );
      QVERIFY( cspy.isValid() );

      triggerChange( 1 );
      triggerChange( 1 ); // will get compressed with the previous change
      triggerChange( 3 );
      QTest::qWait( 500 ); // enter event loop and wait for change notifications from the server

      QCOMPARE( spy.count(), 0 );
      QVERIFY( !cspy.isEmpty() );
      delete rec;

      // Test loading changes from the file

      rec = createChangeRecorder();
      QVERIFY( !rec->isEmpty() );

      QSignalSpy spy2( rec, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)) );
      QVERIFY( spy2.isValid() );
      replayNextAndProcess(rec, 1);
      QVERIFY( !rec->isEmpty() );
      replayNextAndProcess(rec, 3);
      QVERIFY( rec->isEmpty() );

      // nothing changes here
      replayNextAndExpectNothing( rec );
      QVERIFY( rec->isEmpty() );
      QCOMPARE( spy2.count(), 2 );
      delete rec;
    }

    void testEmptyChangeReplay()
    {
      ChangeRecorder recorder;
      recorder.setAllMonitored();
      recorder.itemFetchScope().fetchFullPayload();
      recorder.itemFetchScope().fetchAllAttributes();

      // Ensure we listen to a signal, otherwise MonitorPrivate::isLazilyIgnored will ignore notifications
      QSignalSpy spy( &recorder, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)) );

      // Nothing to replay, should emit that signal then.
      replayNextAndExpectNothing( &recorder );

      // Give it something to replay
      triggerChange( 2 );
      QVERIFY( QTest::kWaitForSignal( &recorder, SIGNAL(changesAdded()), 1000 ) );

      replayNextAndProcess( &recorder, 2 );

      // Nothing else to replay now
      replayNextAndExpectNothing( &recorder );
      QVERIFY( recorder.isEmpty() );
    }

  private:
    void triggerChange( Akonadi::Item::Id uid )
    {
      Item item( uid );
      item.setFlag( "random_flag" );
      ItemModifyJob *job = new ItemModifyJob( item );
      job->disableRevisionCheck();
      AKVERIFYEXEC( job );
      item.clearFlag( "random_flag" );
      job = new ItemModifyJob( item );
      job->disableRevisionCheck();
      AKVERIFYEXEC( job );
    }

    void replayNextAndProcess( ChangeRecorder* rec, Akonadi::Item::Id expectedUid )
    {
      kDebug();

      QSignalSpy nothingSpy( rec, SIGNAL(nothingToReplay()) );
      QVERIFY( nothingSpy.isValid() );
      QSignalSpy itemChangedSpy( rec, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)) );
      QVERIFY( itemChangedSpy.isValid() );

      rec->replayNext();
      if ( itemChangedSpy.isEmpty() )
        QVERIFY( QTest::kWaitForSignal( rec, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)), 1000 ) );
      QCOMPARE( itemChangedSpy.count(), 1 );
      QCOMPARE( itemChangedSpy.at(0).at(0).value<Akonadi::Item>().id(), expectedUid );

      rec->changeProcessed();

      QCOMPARE( nothingSpy.count(), 0 );
    }

    void replayNextAndExpectNothing( ChangeRecorder* rec )
    {
      kDebug();

      QSignalSpy nothingSpy( rec, SIGNAL(nothingToReplay()) );
      QVERIFY( nothingSpy.isValid() );
      QSignalSpy itemChangedSpy( rec, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)) );
      QVERIFY( itemChangedSpy.isValid() );

      rec->replayNext(); // emits nothingToReplay immediately

      QCOMPARE( itemChangedSpy.count(), 0 );
      QCOMPARE( nothingSpy.count(), 1 );
    }

    ChangeRecorder* createChangeRecorder() const
    {
      ChangeRecorder* rec = new ChangeRecorder();
      rec->setConfig( settings );
      rec->setAllMonitored();
      rec->itemFetchScope().fetchFullPayload();
      rec->itemFetchScope().fetchAllAttributes();
      return rec;
    }

    QSettings* settings;

};

QTEST_AKONADIMAIN( ChangeRecorderTest, NoGUI )

#include "changerecordertest.moc"
