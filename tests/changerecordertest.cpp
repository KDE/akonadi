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
#include <akonadi/agentmanager.h>

#include <QtCore/QObject>
#include <QtCore/QSettings>

#include <qtest_akonadi.h>

using namespace Akonadi;

Q_DECLARE_METATYPE(QSet<QByteArray>)

class ChangeRecorderTest : public QObject
{
  Q_OBJECT
  private:
    void triggerChange( int uid )
    {
      Item item( uid );
      item.setFlag( "random_flag" );
      ItemModifyJob *job = new ItemModifyJob( item );
      job->disableRevisionCheck();
      QVERIFY( job->exec() );
      item.clearFlag( "random_flag" );
      job = new ItemModifyJob( item );
      job->disableRevisionCheck();
      QVERIFY( job->exec() );
    }

  private slots:
    void initTestCase()
    {
      qRegisterMetaType<Akonadi::Item>();
      qRegisterMetaType<QSet<QByteArray> >();

      // switch all resources offline to reduce interference from them
      foreach ( Akonadi::AgentInstance agent, Akonadi::AgentManager::self()->instances() )
        agent.setIsOnline( false );
    }

    void testChangeRecorder()
    {
      QSettings *settings = new QSettings( "kde.org", "akonadi-changerecordertest", this );
      settings->clear();

      ChangeRecorder *rec = new ChangeRecorder();
      rec->setConfig( settings );
      rec->setAllMonitored();

      QSignalSpy spy( rec, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)) );
      QVERIFY( spy.isValid() );
      QSignalSpy cspy( rec, SIGNAL(changesAdded()) );
      QVERIFY( cspy.isValid() );

      triggerChange( 1 );
      triggerChange( 1 );
      triggerChange( 3 );
      QTest::qWait( 1000 ); // enter event loop and wait for change notifications from the server

      QCOMPARE( spy.count(), 0 );
      QVERIFY( !cspy.isEmpty() );
      delete rec;

      rec = new ChangeRecorder();
      rec->setConfig( settings );
      rec->setAllMonitored();
      rec->itemFetchScope().fetchFullPayload();
      rec->itemFetchScope().fetchAllAttributes();
      QVERIFY( !rec->isEmpty() );

      QSignalSpy spy2( rec, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)) );
      QVERIFY( spy2.isValid() );
      rec->replayNext();
      rec->changeProcessed();
      QVERIFY( !rec->isEmpty() );
      QTest::qWait( 1000 );
      QCOMPARE( spy2.count(), 1 );
      rec->replayNext();
      rec->changeProcessed();
      QVERIFY( rec->isEmpty() );
      QTest::qWait( 1000 );
      QCOMPARE( spy2.count(), 2 );

      rec->replayNext();
      rec->changeProcessed();
      QVERIFY( rec->isEmpty() );
      QTest::qWait( 1000 );
      QCOMPARE( spy2.count(), 2 );
      delete rec;
    }
};

QTEST_AKONADIMAIN( ChangeRecorderTest, NoGUI )

#include "changerecordertest.moc"
