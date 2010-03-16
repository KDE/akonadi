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
#include "public_etm.h"

class EntityTreeModelTest : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();

  void init();

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

#include "entitytreemodeltest.moc"

QTEST_KDEMAIN(EntityTreeModelTest, NoGUI)

