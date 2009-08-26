/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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


#include <qtest_kde.h>

#include <QTimer>

#include "fakeserver.h"
#include "fakesession.h"
#include "fakemonitor.h"
#include "akonadieventqueue.h"
#include "modelspy.h"

#include "imapparser_p.h"


#include "entitytreemodel.h"
#include <entitydisplayattribute.h>
#include <KStandardDirs>



class EntityTreeModelTest : public QObject
{
  Q_OBJECT

protected:
  Collection getCollection(const Collection parent,
                           const QString &name,
                           const QByteArray displayName = QByteArray(),
                           const QByteArray displayDecoration = QByteArray(),
                           const QByteArray resourceName = "testresource1",
                           const QStringList &mimetypeList = QStringList() << "inode/directory" << "application/x-vnd.akonadi-testitem" )
  {
    Collection col;
    col.setName( name );
    col.setId( getCollectionId() );
    col.setRemoteId( name + "RemoteId" );
    col.setContentMimeTypes( mimetypeList );
    col.setParentCollection( parent );
    col.setResource( resourceName );

    EntityDisplayAttribute *eda = col.attribute<EntityDisplayAttribute>( Entity::AddIfMissing );
    if (!displayName.isEmpty())
      eda->setDisplayName( displayName );
    else
      eda->setDisplayName( name );

    if (!displayDecoration.isEmpty())
      eda->setIconName( displayDecoration );
    else
      eda->setIconName( "x-akonadi-test-icon" );

    return col;

  }

  Entity::Id getCollectionId() { return m_collectionId++; }

protected slots:
  void eventProcessed()
  {
    // This is called after the last real event has been
    // started, but it might not be finihsed yet, so we give it a second.
    if ( !m_eventQueue->head()->isTerminalEvent() )
      QTimer::singleShot(1000, this, SLOT(delayedExit()) );
  }

  void delayedExit()
  {
    m_eventLoop->exit();
  }


private slots:
  void initTestCase();

  void init();

  void testCollectionFetch();

private:
  EntityTreeModel *m_model;
  QEventLoop *m_eventLoop;
  EventQueue *m_eventQueue;

  ModelSpy *m_modelSpy;
  FakeAkonadiServer *m_fakeServer;
  FakeSession *m_fakeSession;
  FakeMonitor *m_fakeMonitor;
  QByteArray m_sessionName;

  Entity::Id m_collectionId;

};


void EntityTreeModelTest::initTestCase()
{

  m_collectionId = 1;
  m_eventQueue = new EventQueue();
  m_sessionName = "EntityTreeModelTest fake session";
  m_fakeSession = new FakeSession( m_sessionName, this);
  connect( m_eventQueue, SIGNAL( dequeued() ), SLOT( eventProcessed() ) );
  m_fakeServer = new FakeAkonadiServer(m_eventQueue, m_fakeSession);
  m_fakeServer->start();
}

void EntityTreeModelTest::init()
{
  FakeMonitor *fakeMonitor = new FakeMonitor( m_eventQueue, m_fakeServer, this);
  m_model = new EntityTreeModel( m_fakeSession, fakeMonitor );
  m_model->setItemPopulationStrategy( EntityTreeModel::NoItemPopulation );

  m_modelSpy = new ModelSpy(this);
  m_modelSpy->setModel(m_model);
  m_modelSpy->startSpying();
}

void EntityTreeModelTest::testCollectionFetch()
{
  SessionEvent *sessionEvent = new SessionEvent(this);
  sessionEvent->setTrigger( "0 LOGIN " + ImapParser::quote( m_sessionName ) + '\n'  );
  sessionEvent->setResponse( "0 OK User logged in\r\n" );

  m_eventQueue->enqueue( sessionEvent );

  sessionEvent = new SessionEvent(this);
  sessionEvent->setTrigger( "1 LIST 0 INF () (STATISTICS true)\n"  );

  Collection::List allCollections;
  Collection::List collectionList;

  collectionList << getCollection(Collection::root(), "Col0");
  collectionList << getCollection(Collection::root(), "Col1");
  collectionList << getCollection(Collection::root(), "Col2");
  collectionList << getCollection(Collection::root(), "Col3");

  QList<Entity::Id> ids;
  foreach (const Collection & c, collectionList)
  {
    ids << c.id();
  }

  sessionEvent->setAffectedCollections( ids );

  sessionEvent->setHasFollowUpResponse( true );
  m_eventQueue->enqueue( sessionEvent );
  allCollections << collectionList;
  collectionList.clear();
  ids.clear();

//   // Give the collection 'col2' four child collections.
//   Collection col2 = allCollections.at( 2 );
//
//   collectionList << getCollection(col2, "Col4");
//   collectionList << getCollection(col2, "Col5");
//   collectionList << getCollection(col2, "Col6");
//   collectionList << getCollection(col2, "Col7");
//
//   foreach (const Collection & c, collectionList)
//   {
//     ids << c.id();
//   }
//
//   MonitorEvent *monitorEvent = new MonitorEvent( this );
//   monitorEvent->setEventType( MonitorEvent::CollectionsAdded );
//   monitorEvent->setAffectedCollections(ids);
//
//   m_eventQueue->enqueue( monitorEvent );
//   allCollections << collectionList;
//   collectionList.clear();
//   ids.clear();


  // Give the collection 'col0' four child collections.
  Collection col0 = allCollections.at( 0 );

  collectionList << getCollection(col0, "Col4");
  collectionList << getCollection(col0, "Col5");
  collectionList << getCollection(col0, "Col6");
  collectionList << getCollection(col0, "Col7");

  foreach (const Collection & c, collectionList)
  {
    ids << c.id();
  }

  sessionEvent = new SessionEvent(this);
  sessionEvent->setAffectedCollections( ids );
  sessionEvent->setHasFollowUpResponse( true );

  m_eventQueue->enqueue( sessionEvent );
  allCollections << collectionList;
  collectionList.clear();


  sessionEvent = new SessionEvent(this);
  sessionEvent->setResponse( "1 OK List completed\r\n" );
  sessionEvent->setHasTrigger( false );
  m_eventQueue->enqueue( sessionEvent );

  m_fakeServer->setCollectionStore( allCollections );

  m_eventQueue->moveToThread( m_fakeServer );

  m_eventLoop = new QEventLoop(this);
  m_eventLoop->exec();

  // From the job event, We got 8 rows about to be inserted signals and 8 rows inserted signals.
  // From the monitor we get an additional pair of signals.
  // We should not be getting collections 1 at a time from the job, but I haven't figured out why
  // that's happening yet.
  QVERIFY( m_modelSpy->size() == 16 );

  for (int i = 0; i < m_modelSpy->size(); ++i)
  {
      kDebug() << i;
    QVERIFY( m_modelSpy->at( i ).at( 0 ) == ( i % 2 == 0 ? RowsAboutToBeInserted : RowsInserted ) );
  }

  kDebug() << m_model->rowCount();
  for (int i = 0; i < m_model->rowCount(); ++i )
    kDebug() << m_model->index(i, 0).data();

  QVERIFY( m_model->rowCount() == 4 );
  QVERIFY( m_model->index(0, 0).data() == "Col3" );
  QVERIFY( m_model->index(1, 0).data() == "Col2" );
  QVERIFY( m_model->index(2, 0).data() == "Col1" );
  QVERIFY( m_model->index(3, 0).data() == "Col0" );

  // Col0 is at row index 3 by the end
  QModelIndex col0Index = m_model->index(3, 0);

  QVERIFY( col0Index.data() == "Col0" );

  QVERIFY( m_model->rowCount( col0Index ) == 4 );
  kDebug() << m_model->index(0, 0, col0Index).data();
  QVERIFY( m_model->index(0, 0, col0Index).data() == "Col7" );
  QVERIFY( m_model->index(1, 0, col0Index).data() == "Col6" );
  QVERIFY( m_model->index(2, 0, col0Index).data() == "Col5" );
  QVERIFY( m_model->index(3, 0, col0Index).data() == "Col4" );




//   sleep(10);
//   QList<Collection::List> collectionChunks;
//   m_fakeSession->firstListJobResult(collectionChunks);
}


// #endif
#include "entitytreemodeltest.moc"

QTEST_KDEMAIN(EntityTreeModelTest, NoGUI)

