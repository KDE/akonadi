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

#include <QtCore/QObject>

#include <qtest_kde.h>

#include <libakonadi/agentmanager.h>

using namespace Akonadi;

class ResourceTest : public QObject
{
  Q_OBJECT
  private slots:
    void testResourceManagement()
    {
      AgentManager *manager = new AgentManager( this );
      QSignalSpy spyAddInstance( manager, SIGNAL(agentInstanceAdded(QString)) );
      QVERIFY( spyAddInstance.isValid() );
      QSignalSpy spyRemoveInstance( manager, SIGNAL(agentInstanceRemoved(QString)) );
      QVERIFY( spyRemoveInstance.isValid() );

      QString instance = manager->createAgentInstance( "non_existing_resource" );
      QVERIFY( instance.isEmpty() );

      QVERIFY( manager->agentTypes().contains( "akonadi_knut_resource_unittest" ) );
      instance = manager->createAgentInstance( "akonadi_knut_resource_unittest" );
      QVERIFY( !instance.isEmpty() );

      QTest::qWait( 2000 );
      QCOMPARE( spyAddInstance.count(), 1 );
      QVERIFY( manager->agentInstances().contains( instance ) );

      manager->removeAgentInstance( instance );
      QTest::qWait( 2000 );
      QCOMPARE( spyRemoveInstance.count(), 1 );
      QVERIFY( !manager->agentInstances().contains( instance ) );

      delete manager;
    }
};

QTEST_KDEMAIN( ResourceTest, NoGUI )

#include "resourcetest.moc"
