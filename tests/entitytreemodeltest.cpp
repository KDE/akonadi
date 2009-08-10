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
#include "fakeserver.h"
#include "fakesession.h"
#include "fakemonitor.h"
// #include "modelspy.h"

#include "entitytreemodel.h"

using namespace Akonadi;

class EntityTreeModelTest : public QObject
{
  Q_OBJECT
private slots:
  void initTestCase();

  void init();

  void testCollectionFetch();

private:
  EntityTreeModel *m_model;
//   ModelSpy *m_modelSpy;
  FakeAkonadiServer *m_fakeServer;
  FakeSession *m_fakeSession;
  FakeMonitor *m_fakeMonitor;
};


void EntityTreeModelTest::initTestCase()
{
  m_fakeServer = new FakeAkonadiServer();
  m_fakeServer->start();

}

void EntityTreeModelTest::init()
{
  FakeSession *fakeSession = new FakeSession( "EntityTreeModelTest fake session", this);
  FakeMonitor *fakeMonitor = new FakeMonitor(this);
  m_model = new EntityTreeModel(fakeSession, fakeMonitor);
//   m_modelSpy = new ModelSpy(this);
//   m_modelSpy->setModel(m_model);

}

void EntityTreeModelTest::testCollectionFetch()
{
  kDebug() << 2;
  QTest::qWait(1000);
//   sleep(10);
//   QList<Collection::List> collectionChunks;
//   m_fakeSession->firstListJobResult(collectionChunks);
}


// #endif
#include "entitytreemodeltest.moc"

QTEST_KDEMAIN(EntityTreeModelTest, NoGUI)

