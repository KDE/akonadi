/*
    Copyright (c) 2007 Robert Zwerus <arzie@dds.nl>

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

#include "benchmarker.h"

#include <QtCore/QDebug>
#include <QtGui/QApplication>
#include <QDir>
#include <QTimer>
#include <QTest>
#include <QDBusInterface>

#include <libakonadi/agentinstancecreatejob.h>
#include <libakonadi/collectionpathresolver.h>
#include <libakonadi/collectiondeletejob.h>
#include <libakonadi/collectionlistjob.h>
#include <libakonadi/itemdeletejob.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kurl.h>

#define WAIT_TIME 100

using namespace Akonadi;

BenchMarker::BenchMarker( const QString &maildir )
{
  manager = new AgentManager( this );
  connect( manager, SIGNAL( agentInstanceRemoved( const QString & ) ), SLOT( agentInstanceRemoved( const QString & ) ) );
  connect( manager, SIGNAL( agentInstanceStatusChanged( const QString &, AgentManager::Status, const QString & ) ),
           SLOT( agentInstanceStatusChanged( const QString &, AgentManager::Status, const QString & ) ) );

  output( "# Description\t\tAccount name\t\tTime\n" );

  qDebug() << "Benchmarking maildir" << maildir;
  currentAccount = maildir;
  testMaildir( maildir );

  qDebug() << "All done.";
  QTimer::singleShot( 1000, this, SLOT( stop() ) );
}

void BenchMarker::stop()
{
  qApp->quit();
}

QString BenchMarker::createAgent( const QString &name )
{
  AgentInstanceCreateJob *job = new AgentInstanceCreateJob( name );
  job->exec();
  const QString instance = job->instanceIdentifier();

  if( job->error() || instance.isEmpty() ) {
    qDebug() << "  Unable to create resource" << name;
    exit( -1 );
  }
  else
    qDebug() << "  Created resource instance" << instance;

  return instance;
}

void BenchMarker::agentInstanceRemoved( const QString &instance )
{
  Q_UNUSED( instance );
  done = true;
  // qDebug() << "agent removed:" << instance;
}

void BenchMarker::agentInstanceStatusChanged( const QString &agentIdentifier, AgentManager::Status status, const QString &message )
{
  //qDebug() << "agent status changed:" << agentIdentifier << status << message ;
  if ( agentIdentifier == currentInstance ) {
    if ( status == AgentManager::Syncing ) {
      //qDebug() << "    " << message;
    }
    if ( status == AgentManager::Ready ) {
      done = true;
    }
  }
}

void BenchMarker::outputStats( const QString &description )
{
  output( description + "\t\t" + currentAccount + "\t\t" + QByteArray::number( timer.elapsed() ) + "\n" );
}

void BenchMarker::output( const QString &message )
{
  QTextStream out( stdout );
  out << message;
}

void BenchMarker::testMaildir( QString dir )
{
  QString instance = createAgent( "akonadi_maildir_resource" );
  currentInstance = instance;
  QTest::qWait( 1000 ); // HACK until resource startup races are fixed

  done = false;
  qDebug() << "  Configuring resource to use " << dir << "as source.";
  QDBusInterface *configIface = new QDBusInterface( "org.kde.Akonadi.Resource." + instance,
      "/Settings", "org.kde.Akonadi.Maildir.Settings", QDBusConnection::sessionBus(), this );
  if ( configIface && configIface->isValid() ) {
    configIface->call( "setPath", dir );
  } else {
    qFatal( "Could not configure instance %s.", qPrintable( instance ) );
  }

  // import the complete email set
  done = false;
  timer.start();
  qDebug() << "  Synchronising resource.";
  manager->agentInstanceSynchronize( instance );
  while(!done)
    QTest::qWait( WAIT_TIME );
  outputStats( "import" );

  // fetch all headers from each folder
  timer.restart();
  qDebug() << "  Listing all headers of every folder.";
  CollectionListJob *clj = new CollectionListJob( Collection::root() , CollectionListJob::Recursive );
  clj->setResource( instance );
  clj->exec();
  Collection::List list = clj->collections();
  foreach ( Collection collection, list ) {
    ItemFetchJob *ifj = new ItemFetchJob( collection, this );
    ifj->addFetchPart( Item::PartEnvelope );
    ifj->exec();
    QByteArray a;
    foreach ( Item item, ifj->items() ) {
      a = item.part( Item::PartEnvelope );
    }
  }
  outputStats( "fullheaderlist" );

  // mark 20% of messages as read
  timer.restart();
  qDebug() << "  Marking 20% of messages as read.";
  CollectionListJob *clj2 = new CollectionListJob( Collection::root() , CollectionListJob::Recursive );
  clj2->setResource( instance );
  clj2->exec();
  Collection::List list2 = clj2->collections();
  foreach ( Collection collection, list2 ) {
    ItemFetchJob *ifj = new ItemFetchJob( collection, this );
    ifj->exec();
    Item::List itemlist = ifj->items();
    for ( int i = ifj->items().count() - 1; i >= 0; i -= 5) {
      Item item = itemlist[i];
      ItemStoreJob *isj = new ItemStoreJob( item, this );
      isj->addFlag( "\\Seen" );
      isj->exec();
    }
  }
  outputStats( "mark20percentread" );

  // fetch headers of unread messages from each folder
  timer.restart();
  qDebug() << "  Listing headers of unread messages of every folder.";
  CollectionListJob *clj3 = new CollectionListJob( Collection::root() , CollectionListJob::Recursive );
  clj3->setResource( instance );
  clj3->exec();
  Collection::List list3 = clj3->collections();
  foreach ( Collection collection, list3 ) {
    ItemFetchJob *ifj = new ItemFetchJob( collection, this );
    ifj->addFetchPart( Item::PartEnvelope );
    ifj->exec();
    QByteArray a;
    foreach ( Item item, ifj->items() ) {
      // filter read messages
      if( !item.hasFlag( "\\Seen" ) ) {
        a = item.part( Item::PartEnvelope );
      }
    }
  }
  outputStats( "unreadheaderlist" );

  // remove all read messages from each folder
  timer.restart();
  qDebug() << "  Removing read messages from every folder.";
  CollectionListJob *clj4 = new CollectionListJob( Collection::root() , CollectionListJob::Recursive );
  clj4->setResource( instance );
  clj4->exec();
  Collection::List list4 = clj4->collections();
  foreach ( Collection collection, list4 ) {
    ItemFetchJob *ifj = new ItemFetchJob( collection, this );
    ifj->exec();
    foreach ( Item item, ifj->items() ) {
      // delete read messages
      if( item.hasFlag( "\\Seen" ) ) {
        ItemDeleteJob *idj = new ItemDeleteJob( item.reference(), this);
        idj->exec();
      }
    }
  }
  outputStats( "removereaditems" );

  // remove every folder sequentially
  timer.restart();
  qDebug() << "  Removing every folder sequentially.";
  CollectionListJob *clj5 = new CollectionListJob( Collection::root() , CollectionListJob::Recursive );
  clj5->setResource( instance );
  clj5->exec();
  Collection::List list5 = clj5->collections();
  foreach ( Collection collection, list5 ) {
    CollectionDeleteJob *cdj = new CollectionDeleteJob( collection, this );
    cdj->exec();
  }
  outputStats( "removeallcollections" );

  // remove resource
  qDebug() << "  Removing resource.";
  manager->removeAgentInstance( instance );
}

int main( int argc, char* argv[] )
{
  KCmdLineArgs::init( argc, argv, "benchmarker", 0, ki18n("Benchmarker") ,"1.0" ,ki18n("benchmark application") );

  KCmdLineOptions options;
  options.add("maildir <argument>", ki18n("Path to maildir to be used as data source"));
  KCmdLineArgs::addCmdLineOptions( options );
  KApplication app( false );
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  QString maildir = args->getOption( "maildir" );

  BenchMarker d( maildir );

  return app.exec();
}

#include "benchmarker.moc"
