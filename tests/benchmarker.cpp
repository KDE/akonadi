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

#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/collectionpathresolver.h>
#include <akonadi/collectiondeletejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemmodifyjob.h>

#include <kmime/kmime_message.h>

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kurl.h>

#include <boost/shared_ptr.hpp>

#define WAIT_TIME 100

typedef boost::shared_ptr<KMime::Message> MessagePtr;

using namespace Akonadi;

BenchMarker::BenchMarker( const QString &maildir )
{
  connect( AgentManager::self(), SIGNAL( instanceRemoved( const AgentInstance& ) ),
           this, SLOT( instanceRemoved( const AgentInstance& ) ) );
  connect( AgentManager::self(), SIGNAL( instanceStatusChanged( const AgentInstance& ) ),
           this, SLOT( instanceStatusChanged( const AgentInstance& ) ) );

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

AgentInstance BenchMarker::createAgent( const QString &name )
{
  const AgentType type = AgentManager::self()->type( name );

  AgentInstanceCreateJob *job = new AgentInstanceCreateJob( type );
  job->exec();
  const AgentInstance instance = job->instance();

  if( job->error() || !instance.isValid() ) {
    qDebug() << "  Unable to create resource" << name;
    exit( -1 );
  }
  else
    qDebug() << "  Created resource instance" << instance.identifier();

  return instance;
}

void BenchMarker::instanceRemoved( const AgentInstance &instance )
{
  Q_UNUSED( instance );
  done = true;
  // qDebug() << "agent removed:" << instance;
}

void BenchMarker::instanceStatusChanged( const AgentInstance &instance )
{
  //qDebug() << "agent status changed:" << agentIdentifier << status << message ;
  if ( instance == currentInstance ) {
    if ( instance.status() == AgentInstance::Syncing ) {
      //qDebug() << "    " << message;
    }
    if ( instance.status() == AgentInstance::Ready ) {
      done = true;
    }
  }
}

void BenchMarker::outputStats( const QString &description )
{
  output( description + "\t\t" + currentAccount + "\t\t" + QByteArray::number( timer.elapsed() ) + '\n' );
}

void BenchMarker::output( const QString &message )
{
  QTextStream out( stdout );
  out << message;
}

void BenchMarker::testMaildir( QString dir )
{
  currentInstance = createAgent( "akonadi_maildir_resource" );
  QTest::qWait( 1000 ); // HACK until resource startup races are fixed

  done = false;
  qDebug() << "  Configuring resource to use " << dir << "as source.";
  QDBusInterface *configIface = new QDBusInterface( "org.kde.Akonadi.Resource." + currentInstance.identifier(),
      "/Settings", "org.kde.Akonadi.Maildir.Settings", QDBusConnection::sessionBus(), this );
  if ( configIface && configIface->isValid() ) {
    configIface->call( "setPath", dir );
    configIface->call( "setReadOnly", true );
  } else {
    qFatal( "Could not configure instance %s.", qPrintable( currentInstance.identifier() ) );
  }

  // import the complete email set
  done = false;
  timer.start();
  qDebug() << "  Synchronising resource.";
  currentInstance.synchronize();
  while(!done)
    QTest::qWait( WAIT_TIME );
  outputStats( "import" );

  // fetch all headers from each folder
  timer.restart();
  qDebug() << "  Listing all headers of every folder.";
  CollectionFetchJob *clj = new CollectionFetchJob( Collection::root() , CollectionFetchJob::Recursive );
  clj->setResource( currentInstance.identifier() );
  clj->exec();
  Collection::List list = clj->collections();
  foreach ( Collection collection, list ) {
    ItemFetchJob *ifj = new ItemFetchJob( collection, this );
    ifj->addFetchPart( Item::PartEnvelope );
    ifj->exec();
    QString a;
    foreach ( Item item, ifj->items() ) {
      a = item.payload<MessagePtr>()->subject()->asUnicodeString();
    }
  }
  outputStats( "fullheaderlist" );

  // mark 20% of messages as read
  timer.restart();
  qDebug() << "  Marking 20% of messages as read.";
  CollectionFetchJob *clj2 = new CollectionFetchJob( Collection::root() , CollectionFetchJob::Recursive );
  clj2->setResource( currentInstance.identifier() );
  clj2->exec();
  Collection::List list2 = clj2->collections();
  foreach ( Collection collection, list2 ) {
    ItemFetchJob *ifj = new ItemFetchJob( collection, this );
    ifj->exec();
    Item::List itemlist = ifj->items();
    for ( int i = ifj->items().count() - 1; i >= 0; i -= 5) {
      Item item = itemlist[i];
      item.setFlag( "\\Seen" );
      ItemModifyJob *isj = new ItemModifyJob( item, this );
      isj->exec();
    }
  }
  outputStats( "mark20percentread" );

  // fetch headers of unread messages from each folder
  timer.restart();
  qDebug() << "  Listing headers of unread messages of every folder.";
  CollectionFetchJob *clj3 = new CollectionFetchJob( Collection::root() , CollectionFetchJob::Recursive );
  clj3->setResource( currentInstance.identifier() );
  clj3->exec();
  Collection::List list3 = clj3->collections();
  foreach ( Collection collection, list3 ) {
    ItemFetchJob *ifj = new ItemFetchJob( collection, this );
    ifj->addFetchPart( Item::PartEnvelope );
    ifj->exec();
    QString a;
    foreach ( Item item, ifj->items() ) {
      // filter read messages
      if( !item.hasFlag( "\\Seen" ) ) {
        a = item.payload<MessagePtr>()->subject()->asUnicodeString();
      }
    }
  }
  outputStats( "unreadheaderlist" );

  // remove all read messages from each folder
  timer.restart();
  qDebug() << "  Removing read messages from every folder.";
  CollectionFetchJob *clj4 = new CollectionFetchJob( Collection::root() , CollectionFetchJob::Recursive );
  clj4->setResource( currentInstance.identifier() );
  clj4->exec();
  Collection::List list4 = clj4->collections();
  foreach ( Collection collection, list4 ) {
    ItemFetchJob *ifj = new ItemFetchJob( collection, this );
    ifj->exec();
    foreach ( Item item, ifj->items() ) {
      // delete read messages
      if( item.hasFlag( "\\Seen" ) ) {
        ItemDeleteJob *idj = new ItemDeleteJob( item, this);
        idj->exec();
      }
    }
  }
  outputStats( "removereaditems" );

  // remove every folder sequentially
  timer.restart();
  qDebug() << "  Removing every folder sequentially.";
  CollectionFetchJob *clj5 = new CollectionFetchJob( Collection::root() , CollectionFetchJob::Recursive );
  clj5->setResource( currentInstance.identifier() );
  clj5->exec();
  Collection::List list5 = clj5->collections();
  foreach ( Collection collection, list5 ) {
    CollectionDeleteJob *cdj = new CollectionDeleteJob( collection, this );
    cdj->exec();
  }
  outputStats( "removeallcollections" );

  // remove resource
  qDebug() << "  Removing resource.";
  AgentManager::self()->removeInstance( currentInstance );
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
