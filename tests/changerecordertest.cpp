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

#include <libakonadi/changerecorder.h>
#include <libakonadi/itemstorejob.h>

#include <QtCore/QObject>
#include <QtCore/QSettings>

#include <qtest_kde.h>

using namespace Akonadi;

class ChangeRecorderTest : public QObject
{
  Q_OBJECT
  private:
    void triggerChange( int uid )
    {
      Item item( DataReference( uid, QString() ) );
      ItemStoreJob *job = new ItemStoreJob( item );
      job->addFlag( "random_flag" );
      job->noRevCheck();
      QVERIFY( job->exec() );
      job = new ItemStoreJob( item );
      job->removeFlag( "random_flag" );
      job->noRevCheck();
      QVERIFY( job->exec() );
    }

  private slots:
    void initTestCase()
    {
      qRegisterMetaType<Akonadi::Item>();
    }

    void testChangeRecorder()
    {
      QSettings *settings = new QSettings( "kde.org", "akonadi-changerecordertest", this );
      settings->clear();

      ChangeRecorder *rec = new ChangeRecorder();
      rec->setConfig( settings );
      rec->monitorAll();

      QSignalSpy spy( rec, SIGNAL(itemChanged(Akonadi::Item,QStringList)) );
      QVERIFY( spy.isValid() );
      QSignalSpy cspy( rec, SIGNAL(changesAdded()) );
      QVERIFY( cspy.isValid() );

      triggerChange( 1 );
      triggerChange( 1 );
      triggerChange( 2 );
      QTest::qWait( 1000 ); // enter event loop and wait for change notifications from the server

      QCOMPARE( spy.count(), 0 );
      QVERIFY( !cspy.isEmpty() );
      delete rec;

      rec = new ChangeRecorder();
      rec->setConfig( settings );
      rec->monitorAll();
      rec->fetchAllParts();
      QVERIFY( !rec->isEmpty() );

      QSignalSpy spy2( rec, SIGNAL(itemChanged(Akonadi::Item,QStringList)) );
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

QTEST_KDEMAIN( ChangeRecorderTest, NoGUI )

#include "changerecordertest.moc"
