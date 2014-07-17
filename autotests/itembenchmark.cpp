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

#include <akonadi/agentinstance.h>
#include <akonadi/agentmanager.h>
#include <akonadi/item.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>

#include "qtest_akonadi.h"
#include "test_utils.h"

#include <QtCore/QDebug>

using namespace Akonadi;

class ItemBenchmark : public QObject
{
  Q_OBJECT
  public Q_SLOTS:
    void createResult( KJob * job )
    {
      Q_ASSERT( job->error() == KJob::NoError );
      Item createdItem = static_cast<ItemCreateJob*>( job )->item();
      mCreatedItems[createdItem.size()].append( createdItem );
    }

    void fetchResult( KJob * job )
    {
      Q_ASSERT( job->error() == KJob::NoError );
    }

    void modifyResult( KJob * job )
    {
      Q_ASSERT( job->error() == KJob::NoError );
    }

  private:
    QMap<int, Item::List> mCreatedItems;

  private Q_SLOTS:
    void initTestCase()
    {
      AkonadiTest::checkTestIsIsolated();
      AkonadiTest::setAllResourcesOffline();
    }

    void data()
    {
      QTest::addColumn<int>( "count" );
      QTest::addColumn<int>( "size" );

      QList<int> counts = QList<int>() << 1 << 10 << 100 << 1000; // << 10000;
      QList<int> sizes = QList<int>() << 0 << 256  << 1024 << 8192 << 32768 << 65536;
      foreach( int count, counts )
        foreach( int size, sizes )
          QTest::newRow( QString::fromLatin1( "%1-%2" ).arg( count ).arg( size ).toLatin1().constData() )
            << count << size;
    }

    void itemBenchmarkCreate_data() { data(); }
    void itemBenchmarkCreate() /// Tests performance of creating items in the cache
    {
      QFETCH( int, count );
      QFETCH( int, size );

      const Collection parent( collectionIdFromPath( "res1/foo" ) );
      QVERIFY( parent.isValid() );

      Item item( "application/octet-stream" );
      item.setPayload( QByteArray( size, 'X' ) );
      item.setSize( size );

      Job *lastJob = 0;
      QBENCHMARK {
        for ( int i = 0; i < count; ++i ) {
          lastJob = new ItemCreateJob( item, parent, this );
          connect( lastJob, SIGNAL(result(KJob*)), SLOT(createResult(KJob*)) );
        }
        QTest::kWaitForSignal( lastJob, SIGNAL(result(KJob*)) );
      }
    }

    void itemBenchmarkFetch_data() { data(); }
    void itemBenchmarkFetch() /// Tests performance of fetching cached items
    {
      QFETCH( int, count );
      QFETCH( int, size );

      // With only one iteration itemBenchmarkCreate() should have created count
      // items, otherwise iterations * count, however, at least count items should
      // be there.
      QVERIFY( mCreatedItems.value( size ).count() >= count );

      QBENCHMARK {
        Item::List items;
        for ( int i = 0; i < count; ++i )
          items << mCreatedItems[size].at(i);

        ItemFetchJob *fetchJob = new ItemFetchJob( items, this );
        fetchJob->fetchScope().fetchFullPayload();
        fetchJob->fetchScope().setCacheOnly( true );
        connect( fetchJob, SIGNAL(result(KJob*)), SLOT(fetchResult(KJob*)) );
        QTest::kWaitForSignal( fetchJob, SIGNAL(result(KJob*)) );
      }
    }

    void itemBenchmarkModifyPayload_data() { data(); }
    void itemBenchmarkModifyPayload() /// Tests performance of modifying payload of cached items
    {
      QFETCH( int, count );
      QFETCH( int, size );

      // With only one iteration itemBenchmarkCreate() should have created count
      // items, otherwise iterations * count, however, at least count items should
      // be there.
      QVERIFY( mCreatedItems.value( size ).count() >= count );

      Job *lastJob = 0;
      const int newSize = qMax(size, 1);
      QBENCHMARK {
        for ( int i = 0; i < count; ++i ) {
          Item item = mCreatedItems.value( size ).at( i );
          item.setPayload( QByteArray( newSize, 'Y' ) );
          ItemModifyJob *job = new ItemModifyJob( item, this );
          job->disableRevisionCheck();
          lastJob = job;
          connect( lastJob, SIGNAL(result(KJob*)), SLOT(modifyResult(KJob*)) );
        }
        QTest::kWaitForSignal( lastJob, SIGNAL(result(KJob*)) );
      }
    }

    void itemBenchmarkDelete_data() { data(); }
    void itemBenchmarkDelete()  /// Tests performance of removing items from the cache
    {
      QFETCH( int, count );
      QFETCH( int, size );

      Job *lastJob = 0;
      int emptyItemArrayIterations = 0;
      QBENCHMARK {
        if ( mCreatedItems[size].isEmpty() )
          ++emptyItemArrayIterations;

        Item::List items;
        for ( int i = 0; i < count && !mCreatedItems[size].isEmpty(); ++i )
          items << mCreatedItems[size].takeFirst();
        lastJob = new ItemDeleteJob( items, this );
        QTest::kWaitForSignal( lastJob, SIGNAL(result(KJob*)) );
      }

      if ( emptyItemArrayIterations )
        qDebug() << "Delete Benchmark performed" << emptyItemArrayIterations << "times on an empty list.";
    }
};

QTEST_AKONADIMAIN( ItemBenchmark, NoGUI )

#include "itembenchmark.moc"
