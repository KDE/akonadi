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

#include <qtest_akonadi.h>


#include "fakeserverdata.h"
#include "fakesession.h"
#include "fakemonitor.h"
#include "modelspy.h"

#include "imapparser_p.h"

#include "entitytreemodel.h"
#include <entitydisplayattribute.h>
#include <entitytreemodel_p.h>

static const QString serverContent1 = QLatin1String(
    // The format of these lines are first a type, either 'C' or 'I' for Item and collection.
    // The dashes show the depth in the heirarchy
    // Collections have a list of mimetypes they can contain, followed by an optional
    // displayName which is put into the EntityDisplayAttribute, followed by an optional order
    // which is the order in which the collections are returned from the job to the ETM.

    "- C (inode/directory)                  'Col 1'     4"
    "- - C (text/directory, message/rfc822) 'Col 2'     3"
    // Items just have the mimetype they contain in the payload.
    "- - - I text/directory                 'Item 1'"
    "- - - I text/directory                 'Item 2'"
    "- - - I message/rfc822                 'Item 3'"
    "- - - I message/rfc822                 'Item 4'"
    "- - C (text/directory)                 'Col 3'     3"
    "- - - C (text/directory)               'Col 4'     2"
    "- - - - C (text/directory)             'Col 5'     1"  // <-- First collection to be returned
    "- - - - - I text/directory             'Item 5'"
    "- - - - - I text/directory             'Item 6'"
    "- - - - I text/directory               'Item 7'"
    "- - - I text/directory                 'Item 8'"
    "- - - I text/directory                 'Item 9'"
    "- - C (message/rfc822)                 'Col 6'     3"
    "- - - I message/rfc822                 'Item 10'"
    "- - - I message/rfc822                 'Item 11'"
    "- - C (text/directory, message/rfc822) 'Col 7'     3"
    "- - - I text/directory                 'Item 12'"
    "- - - I text/directory                 'Item 13'"
    "- - - I message/rfc822                 'Item 14'"
    "- - - I message/rfc822                 'Item 15'");

/**
 * This test verifies that the ETM reacts as expected to signals from the monitor.
 *
 * The tested ETM is only talking to fake components so the reaction of the ETM to each signal can be tested.
 *
 * WARNING: This test does no handle jobs issued by the model. It simply shortcuts (calls emitResult) them, and the connected
 * slots are never executed (because the eventloop is not run after emitResult is called).
 * This test therefore only tests the reaction of the model to signals of the monitor and not the overall behaviour.
 */
class EntityTreeModelTest : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void initTestCase();

  void testInitialFetch();
  void testCollectionMove_data();
  void testCollectionMove();
  void testCollectionAdded_data();
  void testCollectionAdded();
  void testCollectionRemoved_data();
  void testCollectionRemoved();
  void testCollectionChanged_data();
  void testCollectionChanged();
  void testItemMove_data();
  void testItemMove();
  void testItemAdded_data();
  void testItemAdded();
  void testItemRemoved_data();
  void testItemRemoved();
  void testItemChanged_data();
  void testItemChanged();
  void testRemoveCollectionOnChanged();

private:
  ExpectedSignal getExpectedSignal( SignalType type, int start, int end, const QVariantList newData)
  {
    return getExpectedSignal( type, start, end, QVariant(), newData );
  }

  ExpectedSignal getExpectedSignal( SignalType type, int start, int end, const QVariant &parentData = QVariant(), const QVariantList newData = QVariantList() )
  {
    ExpectedSignal expectedSignal;
    expectedSignal.signalType = type;
    expectedSignal.startRow = start;
    expectedSignal.endRow = end;
    expectedSignal.parentData = parentData;
    expectedSignal.newData = newData;
    return expectedSignal;
  }

  ExpectedSignal getExpectedSignal( SignalType type, int start, int end, const QVariant &sourceParentData, int destRow, const QVariant &destParentData, const QVariantList newData)
  {
    ExpectedSignal expectedSignal;
    expectedSignal.signalType = type;
    expectedSignal.startRow = start;
    expectedSignal.endRow = end;
    expectedSignal.sourceParentData = sourceParentData;
    expectedSignal.destRow = destRow;
    expectedSignal.parentData = destParentData;
    expectedSignal.newData = newData;
    return expectedSignal;
  }

  QPair<FakeServerData*, Akonadi::EntityTreeModel*> populateModel( const QString &serverContent, const QString &mimeType = QString() )
  {
    FakeMonitor *fakeMonitor = new FakeMonitor(this);

    fakeMonitor->setSession( m_fakeSession );
    fakeMonitor->setCollectionMonitored(Collection::root());
    if (!mimeType.isEmpty()) {
      fakeMonitor->setMimeTypeMonitored(mimeType);
    }
    EntityTreeModel *model = new EntityTreeModel( fakeMonitor, this );

    m_modelSpy = new ModelSpy(this);
    m_modelSpy->setModel(model);

    FakeServerData *serverData = new FakeServerData( model, m_fakeSession, fakeMonitor );
    QList<FakeAkonadiServerCommand *> initialFetchResponse =  FakeJobResponse::interpret( serverData, serverContent );
    serverData->setCommands( initialFetchResponse );

    // Give the model a chance to populate
    QTest::qWait(100);
    return qMakePair( serverData, model );
  }

private:
  ModelSpy *m_modelSpy;
  FakeSession *m_fakeSession;
  QByteArray m_sessionName;

};

void EntityTreeModelTest::initTestCase()
{
  m_sessionName = "EntityTreeModelTest fake session";
  m_fakeSession = new FakeSession( m_sessionName, FakeSession::EndJobsImmediately);
  m_fakeSession->setAsDefaultSession();

  qRegisterMetaType<QModelIndex>("QModelIndex");
}

void EntityTreeModelTest::testInitialFetch()
{
  FakeMonitor *fakeMonitor = new FakeMonitor(this);

  fakeMonitor->setSession( m_fakeSession );
  fakeMonitor->setCollectionMonitored(Collection::root());
  EntityTreeModel *model = new EntityTreeModel( fakeMonitor, this );

  FakeServerData *serverData = new FakeServerData( model, m_fakeSession, fakeMonitor );
  QList<FakeAkonadiServerCommand *> initialFetchResponse =  FakeJobResponse::interpret( serverData, serverContent1 );
  serverData->setCommands( initialFetchResponse );

  m_modelSpy = new ModelSpy(this);
  m_modelSpy->setModel(model);
  m_modelSpy->startSpying();

  QList<ExpectedSignal> expectedSignals;

  // First the model gets a signal about the first collection to be returned, which is not a top-level collection.
  // It uses the parentCollection heirarchy to put placeholder collections in the model until the root is reached.
  // Then it inserts only one row and emits the correct signals. After that, when the other collections
  // arrive, dataChanged is emitted for them.

  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, 0, 0 );
  expectedSignals << getExpectedSignal( RowsInserted, 0, 0 );
  expectedSignals << getExpectedSignal( DataChanged, 0, 0, QVariantList() << QLatin1String("Col 4") );
  expectedSignals << getExpectedSignal( DataChanged, 0, 0, QVariantList() << QLatin1String("Col 3") );
  // New collections are prepended
  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, 0, 0, QLatin1String("Collection 1") );
  expectedSignals << getExpectedSignal( RowsInserted, 0, 0, QLatin1String("Collection 1"), QVariantList() << QLatin1String("Col 2") );
  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, 0, 0, QLatin1String("Collection 1") );
  expectedSignals << getExpectedSignal( RowsInserted, 0, 0, QLatin1String("Collection 1"), QVariantList() << QLatin1String("Col 6") );
  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, 0, 0, QLatin1String("Collection 1") );
  expectedSignals << getExpectedSignal( RowsInserted, 0, 0, QLatin1String("Collection 1"), QVariantList() << QLatin1String("Col 7") );
  expectedSignals << getExpectedSignal( DataChanged, 0, 0, QVariantList() << QLatin1String("Col 1") );
  // The items in the collections are appended.
  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, 0, 3, QLatin1String("Col 2") );
  expectedSignals << getExpectedSignal( RowsInserted, 0, 3, QLatin1String("Col 2") );
  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, 0, 1, QLatin1String("Col 5") );
  expectedSignals << getExpectedSignal( RowsInserted, 0, 1, QLatin1String("Col 5") );
  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, 1, 1, QLatin1String("Col 4") );
  expectedSignals << getExpectedSignal( RowsInserted, 1, 1, QLatin1String("Col 4") );
  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, 1, 2, QLatin1String("Col 3") );
  expectedSignals << getExpectedSignal( RowsInserted, 1, 2, QLatin1String("Col 3") );
  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, 0, 1, QLatin1String("Col 6") );
  expectedSignals << getExpectedSignal( RowsInserted, 0, 1, QLatin1String("Col 6") );
  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, 0, 3, QLatin1String("Col 7") );
  expectedSignals << getExpectedSignal( RowsInserted, 0, 3, QLatin1String("Col 7") );
  expectedSignals << getExpectedSignal( DataChanged, 0, 0, QVariantList() << QLatin1String("Col 1") );
  expectedSignals << getExpectedSignal( DataChanged, 3, 3, QVariantList() << QLatin1String("Col 3") );
  expectedSignals << getExpectedSignal( DataChanged, 0, 0, QVariantList() << QLatin1String("Col 4") );
  expectedSignals << getExpectedSignal( DataChanged, 0, 0, QVariantList() << QLatin1String("Col 5") );
  expectedSignals << getExpectedSignal( DataChanged, 2, 2, QVariantList() << QLatin1String("Col 2") );
  expectedSignals << getExpectedSignal( DataChanged, 1, 1, QVariantList() << QLatin1String("Col 6") );
  expectedSignals << getExpectedSignal( DataChanged, 0, 0, QVariantList() << QLatin1String("Col 7") );

  m_modelSpy->setExpectedSignals( expectedSignals );

  // Give the model a chance to run the event loop to process the signals.
  QTest::qWait(10);

  // We get all the signals we expected.
  QTRY_VERIFY(m_modelSpy->expectedSignals().isEmpty());

  QTest::qWait(10);
  // We didn't get signals we didn't expect.
  QVERIFY( m_modelSpy->isEmpty() );
}

void EntityTreeModelTest::testCollectionMove_data()
{
  QTest::addColumn<QString>( "serverContent" );
  QTest::addColumn<QString>( "movedCollection" );
  QTest::addColumn<QString>( "targetCollection" );

  QTest::newRow("move-collection01") << serverContent1 << "Col 5" << "Col 1";
  QTest::newRow("move-collection02") << serverContent1 << "Col 5" << "Col 2";
  QTest::newRow("move-collection03") << serverContent1 << "Col 5" << "Col 3";
  QTest::newRow("move-collection04") << serverContent1 << "Col 5" << "Col 6";
  QTest::newRow("move-collection05") << serverContent1 << "Col 5" << "Col 7";
  QTest::newRow("move-collection06") << serverContent1 << "Col 3" << "Col 2";
  QTest::newRow("move-collection07") << serverContent1 << "Col 3" << "Col 6";
  QTest::newRow("move-collection08") << serverContent1 << "Col 3" << "Col 7";
  QTest::newRow("move-collection09") << serverContent1 << "Col 7" << "Col 2";
  QTest::newRow("move-collection10") << serverContent1 << "Col 7" << "Col 5";
  QTest::newRow("move-collection11") << serverContent1 << "Col 7" << "Col 4";
  QTest::newRow("move-collection12") << serverContent1 << "Col 7" << "Col 3";
}

void EntityTreeModelTest::testCollectionMove()
{
  QFETCH( QString, serverContent );
  QFETCH( QString, movedCollection );
  QFETCH( QString, targetCollection );

  QPair<FakeServerData*, Akonadi::EntityTreeModel*> testDrivers = populateModel( serverContent );
  FakeServerData *serverData = testDrivers.first;
  Akonadi::EntityTreeModel *model = testDrivers.second;

  QModelIndexList list = model->match( model->index( 0, 0 ), Qt::DisplayRole, movedCollection, 1, Qt::MatchRecursive );
  Q_ASSERT( !list.isEmpty() );
  QModelIndex movedIndex = list.first();
  QString sourceCollection = movedIndex.parent().data().toString();
  int sourceRow = movedIndex.row();

  FakeCollectionMovedCommand *moveCommand = new FakeCollectionMovedCommand( movedCollection, sourceCollection, targetCollection, serverData );

  m_modelSpy->startSpying();
  serverData->setCommands( QList<FakeAkonadiServerCommand*>() << moveCommand );

  QList<ExpectedSignal> expectedSignals;

  expectedSignals << getExpectedSignal( RowsAboutToBeMoved, sourceRow, sourceRow, sourceCollection, 0, targetCollection, QVariantList() << movedCollection );
  expectedSignals << getExpectedSignal( RowsMoved, sourceRow, sourceRow, sourceCollection, 0, targetCollection , QVariantList() << movedCollection );

  m_modelSpy->setExpectedSignals( expectedSignals );
  serverData->processNotifications();

  // Give the model a change to run the event loop to process the signals.
  QTest::qWait(0);

  QVERIFY( m_modelSpy->isEmpty() );
}

void EntityTreeModelTest::testCollectionAdded_data()
{
  QTest::addColumn<QString>( "serverContent" );
  QTest::addColumn<QString>( "addedCollection" );
  QTest::addColumn<QString>( "parentCollection" );

  QTest::newRow("add-collection01") << serverContent1 << "new Collection" << "Col 1";
  QTest::newRow("add-collection02") << serverContent1 << "new Collection" << "Col 2";
  QTest::newRow("add-collection03") << serverContent1 << "new Collection" << "Col 3";
  QTest::newRow("add-collection04") << serverContent1 << "new Collection" << "Col 4";
  QTest::newRow("add-collection05") << serverContent1 << "new Collection" << "Col 5";
  QTest::newRow("add-collection06") << serverContent1 << "new Collection" << "Col 6";
  QTest::newRow("add-collection07") << serverContent1 << "new Collection" << "Col 7";
}

void EntityTreeModelTest::testCollectionAdded()
{
  QFETCH( QString, serverContent );
  QFETCH( QString, addedCollection );
  QFETCH( QString, parentCollection );

  QPair<FakeServerData*, Akonadi::EntityTreeModel*> testDrivers = populateModel( serverContent );
  FakeServerData *serverData = testDrivers.first;

  FakeCollectionAddedCommand *addCommand = new FakeCollectionAddedCommand( addedCollection, parentCollection, serverData );

  m_modelSpy->startSpying();
  serverData->setCommands( QList<FakeAkonadiServerCommand*>() << addCommand );

  QList<ExpectedSignal> expectedSignals;

  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, 0, 0, parentCollection, QVariantList() << addedCollection );
  expectedSignals << getExpectedSignal( RowsInserted, 0, 0, parentCollection, QVariantList() << addedCollection );

  m_modelSpy->setExpectedSignals( expectedSignals );
  serverData->processNotifications();

  // Give the model a change to run the event loop to process the signals.
  QTest::qWait(0);

  QVERIFY( m_modelSpy->isEmpty() );
}

void EntityTreeModelTest::testCollectionRemoved_data()
{
  QTest::addColumn<QString>( "serverContent" );
  QTest::addColumn<QString>( "removedCollection" );

  // The test suite doesn't handle this case yet.
//   QTest::newRow("remove-collection01") << serverContent1 << "Col 1";
  QTest::newRow("remove-collection02") << serverContent1 << "Col 2";
  QTest::newRow("remove-collection03") << serverContent1 << "Col 3";
  QTest::newRow("remove-collection04") << serverContent1 << "Col 4";
  QTest::newRow("remove-collection05") << serverContent1 << "Col 5";
  QTest::newRow("remove-collection06") << serverContent1 << "Col 6";
  QTest::newRow("remove-collection07") << serverContent1 << "Col 7";
}

void EntityTreeModelTest::testCollectionRemoved()
{
  QFETCH( QString, serverContent );
  QFETCH( QString, removedCollection );

  QPair<FakeServerData*, Akonadi::EntityTreeModel*> testDrivers = populateModel( serverContent );
  FakeServerData *serverData = testDrivers.first;
  Akonadi::EntityTreeModel *model = testDrivers.second;

  QModelIndexList list = model->match( model->index( 0, 0 ), Qt::DisplayRole, removedCollection, 1, Qt::MatchRecursive );
  Q_ASSERT( !list.isEmpty() );
  QModelIndex removedIndex = list.first();
  QString parentCollection = removedIndex.parent().data().toString();
  int sourceRow = removedIndex.row();

  FakeCollectionRemovedCommand *removeCommand = new FakeCollectionRemovedCommand( removedCollection, parentCollection, serverData );

  m_modelSpy->startSpying();
  serverData->setCommands( QList<FakeAkonadiServerCommand*>() << removeCommand );

  QList<ExpectedSignal> expectedSignals;

  expectedSignals << getExpectedSignal( RowsAboutToBeRemoved, sourceRow, sourceRow, parentCollection, QVariantList() << removedCollection );
  expectedSignals << getExpectedSignal( RowsRemoved, sourceRow, sourceRow, parentCollection , QVariantList() << removedCollection );

  m_modelSpy->setExpectedSignals( expectedSignals );
  serverData->processNotifications();

  // Give the model a change to run the event loop to process the signals.
  QTest::qWait(0);

  QVERIFY( m_modelSpy->isEmpty() );
}

void EntityTreeModelTest::testCollectionChanged_data()
{
  QTest::addColumn<QString>( "serverContent" );
  QTest::addColumn<QString>( "collectionName" );
  QTest::addColumn<QString>( "monitoredMimeType" );

  QTest::newRow("change-collection01") << serverContent1 << "Col 1" << QString();
  QTest::newRow("change-collection02") << serverContent1 << "Col 2" << QString();
  QTest::newRow("change-collection03") << serverContent1 << "Col 3" << QString();
  QTest::newRow("change-collection04") << serverContent1 << "Col 4" << QString();
  QTest::newRow("change-collection05") << serverContent1 << "Col 5" << QString();
  QTest::newRow("change-collection06") << serverContent1 << "Col 6" << QString();
  QTest::newRow("change-collection07") << serverContent1 << "Col 7" << QString();
  //Don't remove the parent due to a missing mimetype
  QTest::newRow("change-collection08") << serverContent1 << "Col 1" << QString::fromLatin1("message/rfc822");
}

void EntityTreeModelTest::testCollectionChanged()
{
  QFETCH( QString, serverContent );
  QFETCH( QString, collectionName );
  QFETCH( QString, monitoredMimeType );

  QPair<FakeServerData*, Akonadi::EntityTreeModel*> testDrivers = populateModel( serverContent, monitoredMimeType );
  FakeServerData *serverData = testDrivers.first;
  Akonadi::EntityTreeModel *model = testDrivers.second;

  QModelIndexList list = model->match( model->index( 0, 0 ), Qt::DisplayRole, collectionName, 1, Qt::MatchRecursive );
  Q_ASSERT( !list.isEmpty() );
  QModelIndex changedIndex = list.first();
  QString parentCollection = changedIndex.parent().data().toString();
  qDebug() << parentCollection;
  int changedRow = changedIndex.row();

  FakeCollectionChangedCommand *changeCommand = new FakeCollectionChangedCommand( collectionName, parentCollection, serverData );

  m_modelSpy->startSpying();
  serverData->setCommands( QList<FakeAkonadiServerCommand*>() << changeCommand );

  QList<ExpectedSignal> expectedSignals;

  expectedSignals << getExpectedSignal( DataChanged, changedRow, changedRow, parentCollection, QVariantList() << collectionName );

  m_modelSpy->setExpectedSignals( expectedSignals );
  serverData->processNotifications();

  // Give the model a change to run the event loop to process the signals.
  QTest::qWait(0);

  QVERIFY( m_modelSpy->isEmpty() );
}

void EntityTreeModelTest::testItemMove_data()
{
  QTest::addColumn<QString>( "serverContent" );
  QTest::addColumn<QString>( "movedItem" );
  QTest::addColumn<QString>( "targetCollection" );

  QTest::newRow( "move-item01" ) << serverContent1 << "Item 1" << "Col 7";
  QTest::newRow( "move-item02" ) << serverContent1 << "Item 5" << "Col 4"; // Move item to grandparent.
  QTest::newRow( "move-item03" ) << serverContent1 << "Item 7" << "Col 5"; // Move item to sibling.
  QTest::newRow( "move-item04" ) << serverContent1 << "Item 8" << "Col 5"; // Move item to nephew
  QTest::newRow( "move-item05" ) << serverContent1 << "Item 8" << "Col 6"; // Move item to uncle
  QTest::newRow( "move-item02" ) << serverContent1 << "Item 5" << "Col 3"; // Move item to great-grandparent.
}

void EntityTreeModelTest::testItemMove()
{
  QFETCH( QString, serverContent );
  QFETCH( QString, movedItem );
  QFETCH( QString, targetCollection );

  QPair<FakeServerData*, Akonadi::EntityTreeModel*> testDrivers = populateModel( serverContent );
  FakeServerData *serverData = testDrivers.first;
  Akonadi::EntityTreeModel *model = testDrivers.second;

  QModelIndexList list = model->match( model->index( 0, 0 ), Qt::DisplayRole, movedItem, 1, Qt::MatchRecursive );
  Q_ASSERT( !list.isEmpty() );
  QModelIndex movedIndex = list.first();
  QString sourceCollection = movedIndex.parent().data().toString();
  int sourceRow = movedIndex.row();

  QModelIndexList targetList = model->match( model->index( 0, 0 ), Qt::DisplayRole, targetCollection, 1, Qt::MatchRecursive );
  Q_ASSERT( !targetList.isEmpty() );
  QModelIndex targetIndex = targetList.first();
  int targetRow = model->rowCount( targetIndex );

  FakeItemMovedCommand *moveCommand = new FakeItemMovedCommand( movedItem, sourceCollection, targetCollection, serverData );

  m_modelSpy->startSpying();
  serverData->setCommands( QList<FakeAkonadiServerCommand*>() << moveCommand );

  QList<ExpectedSignal> expectedSignals;

  //Currently moves are implemented as remove + insert in the ETM.
  expectedSignals << getExpectedSignal( RowsAboutToBeRemoved, sourceRow, sourceRow, sourceCollection, QVariantList() << movedItem );
  expectedSignals << getExpectedSignal( RowsRemoved, sourceRow, sourceRow, sourceCollection, QVariantList() << movedItem );
  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, targetRow, targetRow, targetCollection, QVariantList() << movedItem );
  expectedSignals << getExpectedSignal( RowsInserted, targetRow, targetRow, targetCollection, QVariantList() << movedItem );
//   expectedSignals << getExpectedSignal( RowsAboutToBeMoved, sourceRow, sourceRow, sourceCollection, targetRow, targetCollection, QVariantList() << movedItem );
//   expectedSignals << getExpectedSignal( RowsMoved, sourceRow, sourceRow, sourceCollection, targetRow, targetCollection, QVariantList() << movedItem );

  m_modelSpy->setExpectedSignals( expectedSignals );
  serverData->processNotifications();

  // Give the model a change to run the event loop to process the signals.
  QTest::qWait(0);

  QVERIFY( m_modelSpy->isEmpty() );
}

void EntityTreeModelTest::testItemAdded_data()
{
  QTest::addColumn<QString>( "serverContent" );
  QTest::addColumn<QString>( "addedItem" );
  QTest::addColumn<QString>( "parentCollection" );

  QTest::newRow( "add-item01" ) << serverContent1 << "new Item" << "Col 1";
  QTest::newRow( "add-item02" ) << serverContent1 << "new Item" << "Col 2";
  QTest::newRow( "add-item03" ) << serverContent1 << "new Item" << "Col 3";
  QTest::newRow( "add-item04" ) << serverContent1 << "new Item" << "Col 4";
  QTest::newRow( "add-item05" ) << serverContent1 << "new Item" << "Col 5";
  QTest::newRow( "add-item06" ) << serverContent1 << "new Item" << "Col 6";
  QTest::newRow( "add-item07" ) << serverContent1 << "new Item" << "Col 7";
}

void EntityTreeModelTest::testItemAdded()
{
  QFETCH( QString, serverContent );
  QFETCH( QString, addedItem );
  QFETCH( QString, parentCollection );

  QPair<FakeServerData*, Akonadi::EntityTreeModel*> testDrivers = populateModel( serverContent );
  FakeServerData *serverData = testDrivers.first;
  Akonadi::EntityTreeModel *model = testDrivers.second;

  QModelIndexList list = model->match( model->index( 0, 0 ), Qt::DisplayRole, parentCollection, 1, Qt::MatchRecursive );
  Q_ASSERT( !list.isEmpty() );
  QModelIndex parentIndex = list.first();
  int targetRow = model->rowCount( parentIndex );

  FakeItemAddedCommand *addedCommand = new FakeItemAddedCommand( addedItem, parentCollection, serverData );

  m_modelSpy->startSpying();

  serverData->setCommands( QList<FakeAkonadiServerCommand*>() << addedCommand );
  QList<ExpectedSignal> expectedSignals;

  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, targetRow, targetRow, parentCollection, QVariantList() << addedItem );
  expectedSignals << getExpectedSignal( RowsInserted, targetRow, targetRow, parentCollection, QVariantList() << addedItem );

  m_modelSpy->setExpectedSignals( expectedSignals );
  serverData->processNotifications();

  // Give the model a change to run the event loop to process the signals.
  QTest::qWait(0);

  QVERIFY( m_modelSpy->isEmpty() );
}

void EntityTreeModelTest::testItemRemoved_data()
{
  QTest::addColumn<QString>( "serverContent" );
  QTest::addColumn<QString>( "removedItem" );

  QTest::newRow( "remove-item01" ) << serverContent1 << "Item 1";
  QTest::newRow( "remove-item02" ) << serverContent1 << "Item 2";
  QTest::newRow( "remove-item03" ) << serverContent1 << "Item 3";
  QTest::newRow( "remove-item04" ) << serverContent1 << "Item 4";
  QTest::newRow( "remove-item05" ) << serverContent1 << "Item 5";
  QTest::newRow( "remove-item06" ) << serverContent1 << "Item 6";
  QTest::newRow( "remove-item07" ) << serverContent1 << "Item 7";
  QTest::newRow( "remove-item08" ) << serverContent1 << "Item 8";
  QTest::newRow( "remove-item09" ) << serverContent1 << "Item 9";
  QTest::newRow( "remove-item10" ) << serverContent1 << "Item 10";
  QTest::newRow( "remove-item11" ) << serverContent1 << "Item 11";
  QTest::newRow( "remove-item12" ) << serverContent1 << "Item 12";
  QTest::newRow( "remove-item13" ) << serverContent1 << "Item 13";
  QTest::newRow( "remove-item14" ) << serverContent1 << "Item 14";
  QTest::newRow( "remove-item15" ) << serverContent1 << "Item 15";
}

void EntityTreeModelTest::testItemRemoved()
{
  QFETCH( QString, serverContent );
  QFETCH( QString, removedItem );

  QPair<FakeServerData*, Akonadi::EntityTreeModel*> testDrivers = populateModel( serverContent );
  FakeServerData *serverData = testDrivers.first;
  Akonadi::EntityTreeModel *model = testDrivers.second;

  QModelIndexList list = model->match( model->index( 0, 0 ), Qt::DisplayRole, removedItem, 1, Qt::MatchRecursive );
  Q_ASSERT( !list.isEmpty() );
  QModelIndex removedIndex = list.first();
  QString sourceCollection = removedIndex.parent().data().toString();
  int sourceRow = removedIndex.row();

  FakeItemRemovedCommand *removeCommand = new FakeItemRemovedCommand( removedItem, sourceCollection, serverData );

  m_modelSpy->startSpying();
  serverData->setCommands( QList<FakeAkonadiServerCommand*>() << removeCommand );

  QList<ExpectedSignal> expectedSignals;

  expectedSignals << getExpectedSignal( RowsAboutToBeRemoved, sourceRow, sourceRow, sourceCollection, QVariantList() << removedItem );
  expectedSignals << getExpectedSignal( RowsRemoved, sourceRow, sourceRow, sourceCollection, QVariantList() << removedItem );

  m_modelSpy->setExpectedSignals( expectedSignals );
  serverData->processNotifications();

  // Give the model a change to run the event loop to process the signals.
  QTest::qWait(0);

  QVERIFY( m_modelSpy->isEmpty() );
}

void EntityTreeModelTest::testItemChanged_data()
{
  QTest::addColumn<QString>( "serverContent" );
  QTest::addColumn<QString>( "changedItem" );

  QTest::newRow( "change-item01" ) << serverContent1 << "Item 1";
  QTest::newRow( "change-item02" ) << serverContent1 << "Item 2";
  QTest::newRow( "change-item03" ) << serverContent1 << "Item 3";
  QTest::newRow( "change-item04" ) << serverContent1 << "Item 4";
  QTest::newRow( "change-item05" ) << serverContent1 << "Item 5";
  QTest::newRow( "change-item06" ) << serverContent1 << "Item 6";
  QTest::newRow( "change-item07" ) << serverContent1 << "Item 7";
  QTest::newRow( "change-item08" ) << serverContent1 << "Item 8";
  QTest::newRow( "change-item09" ) << serverContent1 << "Item 9";
  QTest::newRow( "change-item10" ) << serverContent1 << "Item 10";
  QTest::newRow( "change-item11" ) << serverContent1 << "Item 11";
  QTest::newRow( "change-item12" ) << serverContent1 << "Item 12";
  QTest::newRow( "change-item13" ) << serverContent1 << "Item 13";
  QTest::newRow( "change-item14" ) << serverContent1 << "Item 14";
  QTest::newRow( "change-item15" ) << serverContent1 << "Item 15";
}

void EntityTreeModelTest::testItemChanged()
{
  QFETCH( QString, serverContent );
  QFETCH( QString, changedItem );

  QPair<FakeServerData*, Akonadi::EntityTreeModel*> testDrivers = populateModel( serverContent );
  FakeServerData *serverData = testDrivers.first;
  Akonadi::EntityTreeModel *model = testDrivers.second;

  QModelIndexList list = model->match( model->index( 0, 0 ), Qt::DisplayRole, changedItem, 1, Qt::MatchRecursive );
  Q_ASSERT( !list.isEmpty() );
  QModelIndex changedIndex = list.first();
  QString parentCollection = changedIndex.parent().data().toString();
  int sourceRow = changedIndex.row();

  FakeItemChangedCommand *changeCommand = new FakeItemChangedCommand( changedItem, parentCollection, serverData );

  m_modelSpy->startSpying();
  serverData->setCommands( QList<FakeAkonadiServerCommand*>() << changeCommand );

  QList<ExpectedSignal> expectedSignals;

  expectedSignals << getExpectedSignal( DataChanged, sourceRow, sourceRow, QVariantList() << changedItem );

  m_modelSpy->setExpectedSignals( expectedSignals );
  serverData->processNotifications();

  // Give the model a change to run the event loop to process the signals.
  QTest::qWait(0);

  QVERIFY( m_modelSpy->isEmpty() );
}

void EntityTreeModelTest::testRemoveCollectionOnChanged()
{
  const QString serverContent = QLatin1String(
    "- C (inode/directory, text/directory)  'Col 1'     1"
    "- - C (text/directory)                 'Col 2'     2"
    "- - - I text/directory                 'Item 1'");
  const QString collectionName = QLatin1String("Col 2");
  const QString monitoredMimeType = QString::fromLatin1("text/directory");

  QPair<FakeServerData*, Akonadi::EntityTreeModel*> testDrivers = populateModel( serverContent, monitoredMimeType );
  FakeServerData *serverData = testDrivers.first;
  Akonadi::EntityTreeModel *model = testDrivers.second;

  QModelIndexList list = model->match( model->index( 0, 0 ), Qt::DisplayRole, collectionName, 1, Qt::MatchRecursive );
  Q_ASSERT( !list.isEmpty() );
  QModelIndex changedIndex = list.first();
  Akonadi::Collection changedCol = changedIndex.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
  changedCol.setContentMimeTypes(QStringList() << QLatin1String("foobar"));
  QString parentCollection = changedIndex.parent().data().toString();

  FakeCollectionChangedCommand *changeCommand = new FakeCollectionChangedCommand( changedCol, serverData );

  m_modelSpy->startSpying();
  serverData->setCommands( QList<FakeAkonadiServerCommand*>() << changeCommand );

  QList<ExpectedSignal> expectedSignals;
  expectedSignals << getExpectedSignal( RowsAboutToBeRemoved, changedIndex.row(), changedIndex.row(), parentCollection, QVariantList() << collectionName );
  expectedSignals << getExpectedSignal( RowsRemoved, changedIndex.row(), changedIndex.row(), parentCollection, QVariantList() << collectionName );

  m_modelSpy->setExpectedSignals( expectedSignals );
  serverData->processNotifications();

  // Give the model a chance to run the event loop to process the signals.
  QTest::qWait(0);

  QVERIFY( m_modelSpy->isEmpty() );
}

#include "entitytreemodeltest.moc"

QTEST_MAIN(EntityTreeModelTest)

