/*
    Copyright (c) 2009 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
    based on kdepimlibs/akonadi/tests/benchmarker.cpp wrote by Robert Zwerus <arzie@dds.nl>

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

#include "test.h"

#include "../../dbusconnectionpool.h"

#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/collectiondeletejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>

#include <QDebug>
#include <QTest>
#include <QDBusInterface>

using namespace Akonadi;

MakeTest::MakeTest()
{
 connect( AgentManager::self(), SIGNAL(instanceRemoved(Akonadi::AgentInstance)),
           this, SLOT(instanceRemoved(Akonadi::AgentInstance)) );
 connect( AgentManager::self(), SIGNAL(instanceStatusChanged(Akonadi::AgentInstance)),
           this, SLOT(instanceStatusChanged(Akonadi::AgentInstance)) );
}

void MakeTest::createAgent(const QString &name)
{
  const AgentType type = AgentManager::self()->type( name );

  AgentInstanceCreateJob *job = new AgentInstanceCreateJob( type );
  job->exec();
  currentInstance = job->instance();

  if ( job->error() || !currentInstance.isValid() ) {
    qDebug() << "  Unable to create resource" << name;
    exit( -1 );
  }
  else
    qDebug() << "  Created resource instance" << currentInstance.identifier();

  QTest::qWait(100); //fix this hack
}

void MakeTest::configureDBusIface(const QString &name,const QString &dir)
{
  QDBusInterface *configIface = new QDBusInterface( "org.freedesktop.Akonadi.Resource." + currentInstance.identifier(),
      "/Settings", "org.kde.Akonadi." + name + ".Settings", DBusConnectionPool::threadConnection(), this );

  configIface->call( "setPath", dir );
  configIface->call( "setReadOnly", true );

  if ( !configIface->isValid())
    qFatal( "Could not configure instance %s.", qPrintable( currentInstance.identifier() ) );
}

void MakeTest::instanceRemoved( const AgentInstance &instance )
{
  Q_UNUSED( instance );
  done = true;
  // qDebug() << "agent removed:" << instance;
}

void MakeTest::instanceStatusChanged( const AgentInstance &instance )
{
  //qDebug() << "agent status changed:" << agentIdentifier << status << message ;
  if ( instance == currentInstance ) {
    if ( instance.status() == AgentInstance::Running ) {
      //qDebug() << "    " << message;
    }
    if ( instance.status() == AgentInstance::Idle ) {
      done = true;
    }
  }
}

void MakeTest::outputStats( const QString &description )
{
  output( description + "\t\t" + currentAccount + "\t\t" + QByteArray::number( timer.elapsed() ) + '\n' );
}

void MakeTest::output( const QString &message )
{
  QTextStream out( stdout );
  out << message;
}

void MakeTest::removeCollections()
{
  timer.restart();
  qDebug() << "  Removing every folder sequentially.";
  CollectionFetchJob *clj5 = new CollectionFetchJob( Collection::root() , CollectionFetchJob::Recursive );
  clj5->fetchScope().setResource( currentInstance.identifier() );
  clj5->exec();
  Collection::List list5 = clj5->collections();
  foreach ( const Collection &collection, list5 ) {
    CollectionDeleteJob *cdj = new CollectionDeleteJob( collection, this );
    cdj->exec();
  }
  outputStats( "removeallcollections" );
}

void MakeTest::removeResource()
{
  qDebug() << "  Removing resource.";
  AgentManager::self()->removeInstance( currentInstance );
}

void MakeTest::start()
{
  runTest();
}
