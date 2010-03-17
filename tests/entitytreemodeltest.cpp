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

#include "fakeserverdata.h"
#include "fakesession.h"
#include "fakemonitor.h"
#include "modelspy.h"

#include "imapparser_p.h"

#include "entitytreemodel.h"
#include <entitydisplayattribute.h>
#include <KStandardDirs>
#include <entitytreemodel_p.h>

static const QString serverContent1 =
    // The format of these lines are first a type, either 'C' or 'I' for Item and collection.
    // The dashes show the depth in the heirarchy
    // Collections have a list of mimetypes they can contain, followed by an optional
    // displayName which is put into the EntityDisplayAttribute, followed by an optional order
    // which is the order in which the collections are returned from the job to the ETM.
    "- C (inode/directory) 'Col 1' 4"
    "- - C (text/directory, message/rfc822) 'Col 2' 3"
    // Items just have the mimetype they contain in the payload.
    "- - - I text/directory"
    "- - - I text/directory 'Item 1'"
    "- - - I message/rfc822"
    "- - - I message/rfc822"
    "- - C (text/directory) 'Col 3' 3"
    "- - - C (text/directory) 'Col 4' 2"
    "- - - - C (text/directory) 'Col 5' 1"  // <-- First collection to be returned
    "- - - - - I text/directory"
    "- - - - - I text/directory"
    "- - - - I text/directory"
    "- - - I text/directory"
    "- - - I text/directory"
    "- - C (message/rfc822) 'Col 6' 3"
    "- - - I message/rfc822 'Item 1'"
    "- - - I message/rfc822"
    "- - C (text/directory, message/rfc822) 'Col 7' 3"
    "- - - I text/directory"
    "- - - I text/directory"
    "- - - I message/rfc822"
    "- - - I message/rfc822";

class EntityTreeModelTest : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();

  void testInitialFetch();
  void testCollectionMove_data();
  void testCollectionMove();
  void testItemMove_data();
  void testItemMove();

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

  QPair<FakeServerData*, Akonadi::EntityTreeModel*> populateModel( const QString &serverContent )
  {
    FakeMonitor *fakeMonitor = new FakeMonitor(this);

    fakeMonitor->setSession( m_fakeSession );
    fakeMonitor->setCollectionMonitored(Collection::root());
    EntityTreeModel *model = new EntityTreeModel( fakeMonitor, this );

    m_modelSpy = new ModelSpy(this);
    m_modelSpy->setModel(model);

    FakeServerData *serverData = new FakeServerData( model, m_fakeSession, fakeMonitor );
    QList<FakeAkonadiServerCommand *> initialFetchResponse =  FakeJobResponse::interpret( serverData, serverContent );
    serverData->setCommands( initialFetchResponse );

    // Give the model a chance to populate
    QTest::qWait(10);
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
  m_fakeSession = new FakeSession( m_sessionName, this);

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
  expectedSignals << getExpectedSignal( DataChanged, 0, 0, QVariantList() << "Col 4" );
  expectedSignals << getExpectedSignal( DataChanged, 0, 0, QVariantList() << "Col 3" );
  // New collections are prepended
  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, 0, 0, "Collection 1" );
  expectedSignals << getExpectedSignal( RowsInserted, 0, 0, "Collection 1", QVariantList() << "Col 2" );
  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, 0, 0, "Collection 1" );
  expectedSignals << getExpectedSignal( RowsInserted, 0, 0, "Collection 1", QVariantList() << "Col 6" );
  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, 0, 0, "Collection 1" );
  expectedSignals << getExpectedSignal( RowsInserted, 0, 0, "Collection 1", QVariantList() << "Col 7" );
  expectedSignals << getExpectedSignal( DataChanged, 0, 0, QVariantList() << "Col 1" );
  // The items in the collections are appended.
  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, 0, 3, "Col 2" );
  expectedSignals << getExpectedSignal( RowsInserted, 0, 3, "Col 2" );
  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, 0, 1, "Col 5" );
  expectedSignals << getExpectedSignal( RowsInserted, 0, 1, "Col 5" );
  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, 1, 1, "Col 4" );
  expectedSignals << getExpectedSignal( RowsInserted, 1, 1, "Col 4" );
  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, 1, 2, "Col 3" );
  expectedSignals << getExpectedSignal( RowsInserted, 1, 2, "Col 3" );
  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, 0, 1, "Col 6" );
  expectedSignals << getExpectedSignal( RowsInserted, 0, 1, "Col 6" );
  expectedSignals << getExpectedSignal( RowsAboutToBeInserted, 0, 3, "Col 7" );
  expectedSignals << getExpectedSignal( RowsInserted, 0, 3, "Col 7" );

  m_modelSpy->setExpectedSignals( expectedSignals );

  // Give the model a chance to run the event loop to process the signals.
  QTest::qWait(10);

  QVERIFY( m_modelSpy->isEmpty() );
}

void EntityTreeModelTest::testCollectionMove_data()
{
  QTest::addColumn<QString>( "serverContent" );
  QTest::addColumn<QString>( "movedCollection" );
  QTest::addColumn<int>( "sourceRow" );
  QTest::addColumn<QString>( "sourceCollection" );
  QTest::addColumn<QString>( "targetCollection" );

  QTest::newRow("move-collection01") << serverContent1 << "Col 4" << 0 << "Col 3" << "Col 7";
}

void EntityTreeModelTest::testCollectionMove()
{
  QFETCH( QString, serverContent );
  QFETCH( QString, movedCollection );
  QFETCH( int, sourceRow );
  QFETCH( QString, sourceCollection );
  QFETCH( QString, targetCollection );

  QPair<FakeServerData*, Akonadi::EntityTreeModel*> testDrivers = populateModel( serverContent );
  FakeServerData *serverData = testDrivers.first;
  Akonadi::EntityTreeModel *model = testDrivers.second;

  FakeCollectionMovedCommand *moveCommand = new FakeCollectionMovedCommand( movedCollection, sourceCollection, targetCollection, model );

  m_modelSpy->startSpying();
  serverData->setCommands( QList<FakeAkonadiServerCommand*>() << moveCommand );

  QList<ExpectedSignal> expectedSignals;

  expectedSignals << getExpectedSignal( RowsAboutToBeMoved, sourceRow, sourceRow, sourceCollection, 0, targetCollection, QVariantList() << movedCollection );
  expectedSignals << getExpectedSignal( RowsMoved, sourceRow, sourceRow, sourceCollection, 0, targetCollection , QVariantList() << movedCollection );

  m_modelSpy->setExpectedSignals( expectedSignals );
  serverData->processNotifications();

  // Give the model a change to run the event loop to process the signals.
  QTest::qWait(10);

  QVERIFY( m_modelSpy->isEmpty() );
}

void EntityTreeModelTest::testItemMove_data()
{
  QTest::addColumn<QString>( "serverContent" );
  QTest::addColumn<QString>( "movedItem" );
  QTest::addColumn<int>( "sourceRow" );
  QTest::addColumn<QString>( "sourceCollection" );
  QTest::addColumn<QString>( "targetCollection" );
  QTest::addColumn<int>( "targetRow" );

  QTest::newRow( "move-item01" ) << serverContent1 << "Item 1" << 0 << "Col 6" << "Col 7" << 4;
}

void EntityTreeModelTest::testItemMove()
{
  QFETCH( QString, serverContent );
  QFETCH( QString, movedItem );
  QFETCH( int, sourceRow );
  QFETCH( QString, sourceCollection );
  QFETCH( QString, targetCollection );
  QFETCH( int, targetRow );

  QPair<FakeServerData*, Akonadi::EntityTreeModel*> testDrivers = populateModel( serverContent );
  FakeServerData *serverData = testDrivers.first;
  Akonadi::EntityTreeModel *model = testDrivers.second;

  FakeItemMovedCommand *moveCommand = new FakeItemMovedCommand( movedItem, sourceCollection, targetCollection, model );

  m_modelSpy->startSpying();
  serverData->setCommands( QList<FakeAkonadiServerCommand*>() << moveCommand );

  QList<ExpectedSignal> expectedSignals;

  expectedSignals << getExpectedSignal( RowsAboutToBeMoved, sourceRow, sourceRow, sourceCollection, targetRow, targetCollection, QVariantList() << movedItem );
  expectedSignals << getExpectedSignal( RowsMoved, sourceRow, sourceRow, sourceCollection, targetRow, targetCollection, QVariantList() << movedItem );

  m_modelSpy->setExpectedSignals( expectedSignals );
  serverData->processNotifications();

  // Give the model a change to run the event loop to process the signals.
  QTest::qWait(10);

  QVERIFY( m_modelSpy->isEmpty() );
}


#include "entitytreemodeltest.moc"

QTEST_KDEMAIN(EntityTreeModelTest, NoGUI)

