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

class EntityTreeModelTest : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();

  void testInitialFetch();

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

private:
  EntityTreeModel *m_model;

  ModelSpy *m_modelSpy;
  FakeSession *m_fakeSession;
  FakeMonitor *m_fakeMonitor;
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
  m_model = new EntityTreeModel( fakeMonitor, this );

  FakeServerData *serverData = new FakeServerData( m_model, m_fakeSession, m_fakeMonitor );
  QList<FakeAkonadiServerCommand *> initialFetchResponse =  FakeJobResponse::interpret( serverData,
    // The format of these lines are first a type, either 'C' or 'I' for Item and collection.
    // The dashes show the depth in the heirarchy
    // Collections have a list of mimetypes they can contain, followed by an optional
    // displayName which is put into the EntityDisplayAttribute, followed by an optional order
    // which is the order in which the collections are returned from the job to the ETM.
    "- C (inode/directory) 'Col 1' 4"
    "- - C (text/directory, message/rfc822) 'Col 2' 3"
    // Items just have the mimetype they contain in the payload.
    "- - - I text/directory"
    "- - - I text/directory"
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
    "- - - I message/rfc822"
    "- - - I message/rfc822"
    "- - C (text/directory, message/rfc822) 'Col 7' 3"
    "- - - I text/directory"
    "- - - I text/directory"
    "- - - I message/rfc822"
    "- - - I message/rfc822"
  );
  serverData->setCommands( initialFetchResponse );

  m_modelSpy = new ModelSpy(this);
  m_modelSpy->setModel(m_model);
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

  m_modelSpy->setExpectedSignals( expectedSignals );

  // Give the model a change to run the event loop to process the signals.
  QTest::qWait(1000);
}

#include "entitytreemodeltest.moc"

QTEST_KDEMAIN(EntityTreeModelTest, NoGUI)

