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

#include "test_utils.h"

#include <akonadi/agentmanager.h>
#include <akonadi/agentinstance.h>
#include <akonadi/control.h>
#include <akonadi/collection.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/entitydisplayattribute.h>

#include "../akonadi/collectionsync.cpp"

#include <krandom.h>

#include <QtCore/QObject>
#include <QSignalSpy>
#include <QEventLoop>

#include <qtest_akonadi.h>
#include <resourceselectjob_p.h>

using namespace Akonadi;

Q_DECLARE_METATYPE( KJob* )

class CollectionSyncTest : public QObject
{
  Q_OBJECT
  private:
    Collection::List fetchCollections( const QString &res )
    {
      CollectionFetchJob *fetch = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive, this );
      fetch->fetchScope().setResource( res );
      fetch->fetchScope().setAncestorRetrieval( CollectionFetchScope::All );
      Q_ASSERT( fetch->exec() );
      Q_ASSERT( !fetch->collections().isEmpty() );
      return fetch->collections();
    }

    void makeTestData()
    {
      QTest::addColumn<bool>( "hierarchicalRIDs" );
      QTest::addColumn<QString>( "resource" );

      QTest::newRow( "akonadi_knut_resource_0 global RID" ) << false << "akonadi_knut_resource_0";
      QTest::newRow( "akonadi_knut_resource_1 global RID" ) << false << "akonadi_knut_resource_1";
      QTest::newRow( "akonadi_knut_resource_2 global RID" ) << false << "akonadi_knut_resource_2";

      QTest::newRow( "akonadi_knut_resource_0 hierarchical RID" ) << true << "akonadi_knut_resource_0";
      QTest::newRow( "akonadi_knut_resource_1 hierarchical RID" ) << true << "akonadi_knut_resource_1";
      QTest::newRow( "akonadi_knut_resource_2 hierarchical RID" ) << true << "akonadi_knut_resource_2";
    }

    Collection createCollection(const QString &name, const QString &remoteId, const Collection &parent)
    {
        Collection c;
        c.setName(name);
        c.setRemoteId(remoteId);
        c.setParentCollection(parent);
        c.setResource("akonadi_knut_resource_0");
        c.setContentMimeTypes(QStringList() << Collection::mimeType());
        return c;
    }

    Collection::List prepareBenchmark()
    {
        Collection::List collections = fetchCollections(QLatin1String("akonadi_knut_resource_0"));

        ResourceSelectJob *resJob = new ResourceSelectJob("akonadi_knut_resource_0");
        Q_ASSERT(resJob->exec());

        Collection root;
        Q_FOREACH (const Collection &col, collections) {
            if (col.parentCollection() == Collection::root()) {
                root = col;
                break;
            }
        }
        Q_ASSERT(root.isValid());

        // we must build on top of existing collections, because only resource is
        // allowed to create top-level collection
        Collection::List baseCollections;
        for (int i = 0; i < 20; ++i) {
            baseCollections << createCollection(QString::fromLatin1("Base Col %1").arg(i), QString::fromLatin1("/baseCol%1").arg(i), root);
        }
        collections += baseCollections;

        const Collection shared = createCollection(QLatin1String("Shared collections"), QLatin1String("/shared"), root);
        baseCollections << shared;
        collections << shared;
        for (int i = 0; i < 10000; ++i) {
            const Collection col = createCollection(QString::fromLatin1("Shared Col %1").arg(i), QString::fromLatin1("/shared%1").arg(i), shared);
              collections << col;
              for (int j = 0; j < 6; ++j) {
                  collections << createCollection(QString::fromLatin1("Shared Subcol %1-%2").arg(i).arg(j), QString::fromLatin1("/shared%1-%2").arg(i).arg(j), col);
              }
        }
        return collections;
    }

    CollectionSync *prepareBenchmarkSyncer(const Collection::List &collections)
    {
        CollectionSync *syncer = new CollectionSync("akonadi_knut_resource_0");
        connect(syncer, SIGNAL(percent(KJob*,ulong)), this, SLOT(syncBenchmarkProgress(KJob*,ulong)));
        syncer->setHierarchicalRemoteIds(false);
        syncer->setRemoteCollections(collections);
        return syncer;
    }

    void cleanupBenchmark(const Collection::List &collections)
    {
        Collection::List baseCols;
        Q_FOREACH(const Collection &col, collections) {
            if (col.remoteId().startsWith(QLatin1String("/baseCol")) || col.remoteId() == QLatin1String("/shared")) {
                baseCols << col;
            }
        }
        Q_FOREACH (const Collection &col, baseCols) {
            CollectionDeleteJob *del = new CollectionDeleteJob(col);
            AKVERIFYEXEC(del);
        }
    }


  public Q_SLOTS:
    void syncBenchmarkProgress(KJob *job, ulong percent)
    {
        Q_UNUSED(job);
        qDebug() << "CollectionSync progress:" <<  percent << "%";
    }

  private Q_SLOTS:
    void initTestCase()
    {
      AkonadiTest::checkTestIsIsolated();
      Control::start();
      AkonadiTest::setAllResourcesOffline();
      qRegisterMetaType<KJob*>();
    }

    void testFullSync_data()
    {
      makeTestData();
    }

    void testFullSync()
    {
      QFETCH( bool, hierarchicalRIDs );
      QFETCH( QString, resource );

      Collection::List origCols = fetchCollections( resource );

      CollectionSync* syncer = new CollectionSync( resource, this );
      syncer->setHierarchicalRemoteIds( hierarchicalRIDs );
      syncer->setRemoteCollections( origCols );
      AKVERIFYEXEC( syncer );

      Collection::List resultCols = fetchCollections( resource );
      QCOMPARE( resultCols.count(), origCols.count() );
    }

    void testFullStreamingSync_data()
    {
      makeTestData();
    }

    void testFullStreamingSync()
    {
      QFETCH( bool, hierarchicalRIDs );
      QFETCH( QString, resource );

      Collection::List origCols = fetchCollections( resource );

      CollectionSync* syncer = new CollectionSync( resource, this );
      syncer->setHierarchicalRemoteIds( hierarchicalRIDs );
      syncer->setAutoDelete( false );
      QSignalSpy spy( syncer, SIGNAL(result(KJob*)) );
      QVERIFY( spy.isValid() );
      syncer->setStreamingEnabled( true );
      QTest::qWait( 10 );
      QCOMPARE( spy.count(), 0 );

      for ( int i = 0; i < origCols.count(); ++i ) {
        Collection::List l;
        l << origCols[i];
        syncer->setRemoteCollections( l );
        if ( i < origCols.count() - 1 )
          QTest::qWait( 10 ); // enter the event loop so itemsync actually can do something
        QCOMPARE( spy.count(), 0 );
      }
      syncer->retrievalDone();
      QTRY_COMPARE( spy.count(), 1 );
      QCOMPARE( spy.count(), 1 );
      KJob *job = spy.at( 0 ).at( 0 ).value<KJob*>();
      QCOMPARE( job, syncer );
      QCOMPARE( job->errorText(), QString() );
      QCOMPARE( job->error(), 0 );

      Collection::List resultCols = fetchCollections( resource );
      QCOMPARE( resultCols.count(), origCols.count() );

      delete syncer;
    }

    void testIncrementalSync_data()
    {
      makeTestData();
    }

    void testIncrementalSync()
    {
      QFETCH( bool, hierarchicalRIDs );
      QFETCH( QString, resource );
      if ( resource == QLatin1String( "akonadi_knut_resource_2" ) )
        QSKIP( "test requires more than one collection", SkipSingle );

      Collection::List origCols = fetchCollections( resource );

      CollectionSync* syncer = new CollectionSync( resource, this );
      syncer->setHierarchicalRemoteIds( hierarchicalRIDs );
      syncer->setRemoteCollections( origCols, Collection::List() );
      AKVERIFYEXEC( syncer );

      Collection::List resultCols = fetchCollections( resource );
      QCOMPARE( resultCols.count(), origCols.count() );

      Collection::List delCols;
      delCols << resultCols.last();
      resultCols.pop_back();


      // ### not implemented yet I guess
#if 0
      Collection colWithOnlyRemoteId;
      colWithOnlyRemoteId.setRemoteId( resultCols.front().remoteId() );
      delCols << colWithOnlyRemoteId;
      resultCols.pop_front();
#endif

#if 0
      // ### should this work?
      Collection colWithRandomRemoteId;
      colWithRandomRemoteId.setRemoteId( KRandom::randomString( 100 ) );
      delCols << colWithRandomRemoteId;
#endif

      syncer = new CollectionSync( resource, this );
      syncer->setRemoteCollections( resultCols, delCols );
      AKVERIFYEXEC( syncer );

      Collection::List resultCols2 = fetchCollections( resource );
      QCOMPARE( resultCols2.count(), resultCols.count() );
    }

    void testIncrementalStreamingSync_data()
    {
      makeTestData();
    }

    void testIncrementalStreamingSync()
    {
      QFETCH( bool, hierarchicalRIDs );
      QFETCH( QString, resource );

      Collection::List origCols = fetchCollections( resource );

      CollectionSync* syncer = new CollectionSync( resource, this );
      syncer->setHierarchicalRemoteIds( hierarchicalRIDs );
      syncer->setAutoDelete( false );
      QSignalSpy spy( syncer, SIGNAL(result(KJob*)) );
      QVERIFY( spy.isValid() );
      syncer->setStreamingEnabled( true );
      QTest::qWait( 10 );
      QCOMPARE( spy.count(), 0 );

      for ( int i = 0; i < origCols.count(); ++i ) {
        Collection::List l;
        l << origCols[i];
        syncer->setRemoteCollections( l, Collection::List() );
        if ( i < origCols.count() - 1 )
          QTest::qWait( 10 ); // enter the event loop so itemsync actually can do something
        QCOMPARE( spy.count(), 0 );
      }
      syncer->retrievalDone();
      QTRY_COMPARE(spy.count(), 1);
      KJob *job = spy.at( 0 ).at( 0 ).value<KJob*>();
      QCOMPARE( job, syncer );
      QCOMPARE( job->errorText(), QString() );
      QCOMPARE( job->error(), 0 );

      Collection::List resultCols = fetchCollections( resource );
      QCOMPARE( resultCols.count(), origCols.count() );

      delete syncer;
    }

    void testEmptyIncrementalSync_data()
    {
      makeTestData();
    }

    void testEmptyIncrementalSync()
    {
      QFETCH( bool, hierarchicalRIDs );
      QFETCH( QString, resource );

      Collection::List origCols = fetchCollections( resource );

      CollectionSync* syncer = new CollectionSync( resource, this );
      syncer->setHierarchicalRemoteIds( hierarchicalRIDs );
      syncer->setRemoteCollections( Collection::List(), Collection::List() );
      AKVERIFYEXEC( syncer );

      Collection::List resultCols = fetchCollections( resource );
      QCOMPARE( resultCols.count(), origCols.count() );
    }

    void testAttributeChanges_data()
    {
      QTest::addColumn<bool>("keepLocalChanges");
      QTest::newRow("keep local changes" ) << true;
      QTest::newRow("overwrite local changes" ) << false;
    }

    void testAttributeChanges()
    {
      QFETCH(bool, keepLocalChanges);
      const QString resource("akonadi_knut_resource_0");
      Collection col = fetchCollections( resource ).first();
      col.attribute<EntityDisplayAttribute>(Akonadi::Entity::AddIfMissing)->setDisplayName("foo");
      col.setContentMimeTypes(QStringList() << Akonadi::Collection::mimeType() << "foo");
      {
        CollectionModifyJob *job = new CollectionModifyJob(col);
        AKVERIFYEXEC(job);
      }

      col.attribute<EntityDisplayAttribute>()->setDisplayName("default");
      col.setContentMimeTypes(QStringList() << Akonadi::Collection::mimeType() << "default");

      CollectionSync* syncer = new CollectionSync( resource, this );
      if (keepLocalChanges) {
          syncer->setKeepLocalChanges(QSet<QByteArray>() << "ENTITYDISPLAY" << "CONTENTMIMETYPES");
      } else {
          syncer->setKeepLocalChanges(QSet<QByteArray>());
      }

      syncer->setRemoteCollections( Collection::List() << col, Collection::List() );
      AKVERIFYEXEC( syncer );

      {
        CollectionFetchJob *job = new CollectionFetchJob(col, Akonadi::CollectionFetchJob::Base);
        AKVERIFYEXEC(job);
        Collection resultCol = job->collections().first();
        if (keepLocalChanges) {
          QCOMPARE( resultCol.displayName(), QString::fromLatin1("foo") );
          QVERIFY(resultCol.contentMimeTypes().contains("foo"));
        } else {
          QCOMPARE( resultCol.displayName(), QString::fromLatin1("default") );
          QVERIFY(resultCol.contentMimeTypes().contains("default"));
        }
      }
    }

// Disabled by default, because they take ~15 minutes to complete
#if 0
    void benchmarkInitialSync()
    {
        const Collection::List collections = prepareBenchmark();

        CollectionSync *syncer = prepareBenchmarkSyncer(collections);

        QBENCHMARK_ONCE {
            AKVERIFYEXEC(syncer);
        }

        cleanupBenchmark(collections);
    }

    void benchmarkIncrementalSync()
    {
        const Collection::List collections = prepareBenchmark();

        // First populate Akonadi with Collections
        CollectionSync *syncer = prepareBenchmarkSyncer(collections);
        AKVERIFYEXEC(syncer);

        // Now create a new syncer to benchmark the incremental sync
        syncer = prepareBenchmarkSyncer(collections);

        QBENCHMARK_ONCE {
            AKVERIFYEXEC(syncer);
        }

        cleanupBenchmark(collections);
    }
#endif
};

QTEST_AKONADIMAIN( CollectionSyncTest, NoGUI )

#include "collectionsynctest.moc"
