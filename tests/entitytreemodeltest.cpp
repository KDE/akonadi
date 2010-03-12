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
#include <entitytreemodel_p.h>
#include "public_etm.h"

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

    if ( !displayDecoration.isEmpty() )
      eda->setIconName( displayDecoration );
    else
      eda->setIconName( "x-akonadi-test-icon" );

    return col;

  }

  Entity::Id getCollectionId() { return m_collectionId++; }

private slots:
  void initTestCase();

  void init();

  void testCollectionFetch();
  void testCollectionAdded();
  void testCollectionMoved();

private:
  PublicETM *m_model;

  ModelSpy *m_modelSpy;
  FakeSession *m_fakeSession;
  FakeMonitor *m_fakeMonitor;
  QByteArray m_sessionName;

  Entity::Id m_collectionId;

};


void EntityTreeModelTest::initTestCase()
{
  m_collectionId = 1;
  m_sessionName = "EntityTreeModelTest fake session";
  m_fakeSession = new FakeSession( m_sessionName, this);

  qRegisterMetaType<QModelIndex>("QModelIndex");
}

void EntityTreeModelTest::init()
{
  FakeMonitor *fakeMonitor = new FakeMonitor(this);

  fakeMonitor->setSession( m_fakeSession );
  fakeMonitor->setCollectionMonitored(Collection::root());
  m_model = new PublicETM( fakeMonitor, this );
  m_model->setItemPopulationStrategy( EntityTreeModel::NoItemPopulation );

  m_modelSpy = new ModelSpy(this);
  m_modelSpy->setModel(m_model);
  m_modelSpy->startSpying();
}

void EntityTreeModelTest::testCollectionFetch()
{
  Collection::List collectionList;

  collectionList << getCollection(Collection::root(), "Col0");
  collectionList << getCollection(Collection::root(), "Col1");
  collectionList << getCollection(Collection::root(), "Col2");
  collectionList << getCollection(Collection::root(), "Col3");

  // Give the collection 'col0' four child collections.
  Collection col0 = collectionList.at( 0 );

  collectionList << getCollection(col0, "Col4");
  collectionList << getCollection(col0, "Col5");
  collectionList << getCollection(col0, "Col6");
  collectionList << getCollection(col0, "Col7");

  Collection col5 = collectionList.at( 5 );

  collectionList << getCollection(col5, "Col8");
  collectionList << getCollection(col5, "Col9");
  collectionList << getCollection(col5, "Col10");
  collectionList << getCollection(col5, "Col11");

  Collection col6 = collectionList.at( 6 );

  collectionList << getCollection(col6, "Col12");
  collectionList << getCollection(col6, "Col13");
  collectionList << getCollection(col6, "Col14");
  collectionList << getCollection(col6, "Col15");

  Collection::List collectionListReversed;

  foreach(const Collection &c, collectionList)
    collectionListReversed.prepend(c);

  // The first list job is started with a single shot.
  // Give it time to set the root collection.
  QTest::qWait(1);
  m_model->privateClass()->collectionsFetched(collectionListReversed);

  QVERIFY( m_modelSpy->size() == 8 );

  for (int i = 0; i < m_modelSpy->size(); ++i)
  {
    QVERIFY( m_modelSpy->at( i ).at( 0 ) == ( i % 2 == 0 ? RowsAboutToBeInserted : RowsInserted ) );
  }

  QVERIFY( m_model->rowCount() == 4 );

  QSet<QString> expectedRowData;
  QSet<QString> rowData;
  expectedRowData << "Col0" << "Col1" << "Col2" << "Col3";
  for ( int row = 0; row < m_model->rowCount(); ++row )
  {
    rowData.insert(m_model->index(row, 0).data().toString());
  }

  QCOMPARE(rowData, expectedRowData);
  rowData.clear();
  expectedRowData.clear();

  // Col0 is at row index 3 by the end
  QModelIndex col0Index = m_model->match(m_model->index(0, 0), Qt::DisplayRole, "Col0", 1).first();

  expectedRowData << "Col4" << "Col5" << "Col6" << "Col7";

  QVERIFY( m_model->rowCount( col0Index ) == 4 );

  for ( int row = 0; row < m_model->rowCount(col0Index); ++row )
  {
    rowData.insert(m_model->index(row, 0, col0Index).data().toString());
  }

  QCOMPARE(rowData, expectedRowData);
  rowData.clear();
  expectedRowData.clear();

  QModelIndex col5Index = m_model->match(m_model->index(0, 0, col0Index), Qt::DisplayRole, "Col5", 1).first();

  expectedRowData << "Col8" << "Col9" << "Col10" << "Col11";

  QVERIFY( m_model->rowCount( col5Index ) == 4 );

  for ( int row = 0; row < m_model->rowCount(col5Index); ++row )
  {
    rowData.insert(m_model->index(row, 0, col5Index).data().toString());
  }

  QCOMPARE(rowData, expectedRowData);
  rowData.clear();
  expectedRowData.clear();

  QModelIndex col6Index = m_model->match(m_model->index(0, 0, col0Index), Qt::DisplayRole, "Col6", 1).first();

  expectedRowData << "Col12" << "Col13" << "Col14" << "Col15";

  QVERIFY( m_model->rowCount( col6Index ) == 4 );

  for ( int row = 0; row < m_model->rowCount(col6Index); ++row )
  {
    rowData.insert(m_model->index(row, 0, col6Index).data().toString());
  }

  QCOMPARE(rowData, expectedRowData);
  rowData.clear();
  expectedRowData.clear();

}


void EntityTreeModelTest::testCollectionAdded()
{
  Collection::List collectionList;

  collectionList << getCollection(Collection::root(), "Col0");
  collectionList << getCollection(Collection::root(), "Col1");
  collectionList << getCollection(Collection::root(), "Col2");
  collectionList << getCollection(Collection::root(), "Col3");

  // Give the collection 'col0' four child collections.
  Collection col0 = collectionList.at( 0 );

  collectionList << getCollection(col0, "Col4");
  collectionList << getCollection(col0, "Col5");
  collectionList << getCollection(col0, "Col6");
  collectionList << getCollection(col0, "Col7");

  Collection col5 = collectionList.at( 5 );

  collectionList << getCollection(col5, "Col8");
  collectionList << getCollection(col5, "Col9");
  collectionList << getCollection(col5, "Col10");
  collectionList << getCollection(col5, "Col11");

  Collection col6 = collectionList.at( 6 );

  collectionList << getCollection(col6, "Col12");
  collectionList << getCollection(col6, "Col13");
  collectionList << getCollection(col6, "Col14");
  collectionList << getCollection(col6, "Col15");

  Collection::List collectionListReversed;

  foreach(const Collection &c, collectionList)
    collectionListReversed.prepend(c);

  // The first list job is started with a single shot.
  // Give it time to set the root collection.
  QTest::qWait(1);

  m_modelSpy->stopSpying();
  m_model->privateClass()->collectionsFetched(collectionListReversed);
  m_modelSpy->startSpying();

  Collection newCol1 = getCollection(Collection::root(), "NewCollection");

  m_model->privateClass()->monitoredCollectionAdded(newCol1, Collection::root());

  QVERIFY(m_modelSpy->size() == 2);
  for (int i = 0; i < m_modelSpy->size(); ++i)
  {
    QVERIFY( m_modelSpy->at( i ).at( 0 ) == ( i % 2 == 0 ? RowsAboutToBeInserted : RowsInserted ) );
  }
  int start = m_modelSpy->at( 1 ).at( 2 ).toInt();
  int end = m_modelSpy->at( 1 ).at( 3 ).toInt();

  QCOMPARE(m_model->rowCount(), 5);
  QCOMPARE(start, end);
  QCOMPARE(m_model->index(start, 0).data().toString(), QLatin1String("NewCollection"));

}

void EntityTreeModelTest::testCollectionMoved()
{
  Collection::List collectionList;

  collectionList << getCollection(Collection::root(), "Col0");
  collectionList << getCollection(Collection::root(), "Col1");
  collectionList << getCollection(Collection::root(), "Col2");
  collectionList << getCollection(Collection::root(), "Col3");

  // Give the collection 'col0' four child collections.
  Collection col0 = collectionList.at( 0 );

  collectionList << getCollection(col0, "Col4");
  collectionList << getCollection(col0, "Col5");
  collectionList << getCollection(col0, "Col6");
  collectionList << getCollection(col0, "Col7");

  Collection col5 = collectionList.at( 5 );

  collectionList << getCollection(col5, "Col8");
  collectionList << getCollection(col5, "Col9");
  collectionList << getCollection(col5, "Col10");
  collectionList << getCollection(col5, "Col11");

  Collection col6 = collectionList.at( 6 );

  collectionList << getCollection(col6, "Col12");
  Collection col13 = getCollection(col6, "Col13");
  collectionList << col13;
  collectionList << getCollection(col6, "Col14");
  collectionList << getCollection(col6, "Col15");

  Collection::List collectionListReversed;

  foreach(const Collection &c, collectionList)
    collectionListReversed.prepend(c);

  // The first list job is started with a single shot.
  // Give it time to set the root collection.
  QTest::qWait(1);

  m_modelSpy->stopSpying();
  m_model->privateClass()->collectionsFetched(collectionListReversed);
  m_modelSpy->startSpying();

  // Move col13 to col5

  col13.setParentCollection( col5 );
  m_model->privateClass()->monitoredCollectionMoved(col13, col6, col5);

  QVERIFY(m_modelSpy->size() == 2);
  for (int i = 0; i < m_modelSpy->size(); ++i)
  {
    QVERIFY( m_modelSpy->at( i ).at( 0 ) == ( i % 2 == 0 ? RowsAboutToBeMoved : RowsMoved ) );
  }

  int start = m_modelSpy->at( 1 ).at( 2 ).toInt();
  int end = m_modelSpy->at( 1 ).at( 3 ).toInt();
  int dest = m_modelSpy->at( 1 ).at( 5 ).toInt();

  QModelIndex src = qvariant_cast<QModelIndex>( m_modelSpy->at( 1 ).at( 1 ) );
  QModelIndex destIdx = qvariant_cast<QModelIndex>( m_modelSpy->at( 1 ).at( 4 ) );

  QCOMPARE(start, end);
  QCOMPARE(src.data().toString(), QLatin1String("Col6"));
  QCOMPARE(destIdx.data().toString(), QLatin1String("Col5"));
  QCOMPARE(m_model->index(dest, 0, destIdx).data().toString(), QLatin1String("Col13"));

}

#include "entitytreemodeltest.moc"

QTEST_KDEMAIN(EntityTreeModelTest, NoGUI)

