/*
    This file is part of libakonadi.

    Copyright (c) 2006 Till Adam <adam@kde.org>
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

#include "resourcebase.h"
#include "agentbase_p.h"

#include "kcrash.h"
#include "resourceadaptor.h"
#include "collectionsync.h"
#include "itemsync.h"
#include "resourcescheduler.h"
#include "tracerinterface.h"
#include "xdgbasedirs.h"

#include <libakonadi/collectionlistjob.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/session.h>
#include <libakonadi/changerecorder.h>

#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <klocale.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QHash>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtDBus/QtDBus>

#include <signal.h>
#include <stdlib.h>

using namespace Akonadi;

static ResourceBase *sResourceBase = 0;

void crashHandler( int signal )
{
  if ( sResourceBase )
    sResourceBase->crashHandler( signal );

  exit( 255 );
}

class Akonadi::ResourceBasePrivate : public AgentBasePrivate
{
  public:
    ResourceBasePrivate( ResourceBase *parent )
      : AgentBasePrivate( parent ),
        mStatusCode( ResourceBase::Ready ),
        mProgress( 0 ),
        online( true ),
        scheduler( 0 )
    {
      mStatusMessage = defaultReadyMessage();
    }

    Q_DECLARE_PUBLIC( ResourceBase )

    void slotDeliveryDone( KJob* job );

    void slotCollectionSyncDone( KJob *job );
    void slotLocalListDone( KJob *job );
    void slotSynchronizeCollection( const Collection &col );
    void slotCollectionListDone( KJob *job );

    void slotItemSyncDone( KJob *job );

    void slotPercent( KJob* job, unsigned long percent );

    QString defaultReadyMessage() const;
    QString defaultSyncingMessage() const;

    QString mName;

    int mStatusCode;
    QString mStatusMessage;

    uint mProgress;
    QString mProgressMessage;

    QHash<Akonadi::Job*, QDBusMessage> pendingReplies;

    bool online;

    // synchronize states
    Collection currentCollection;

    ResourceScheduler *scheduler;
};

QString ResourceBasePrivate::defaultReadyMessage() const
{
  if ( online )
    return i18nc( "@info:status, application ready for work", "Ready" );
  return i18nc( "@info:status", "Offline" );
}

QString ResourceBasePrivate::defaultSyncingMessage() const
{
  return i18nc( "@info:status", "Syncing..." );
}

ResourceBase::ResourceBase( const QString & id )
  : AgentBase( new ResourceBasePrivate( this ), id )
{
  Q_D( ResourceBase );
  KCrash::init();
  KCrash::setEmergencyMethod( ::crashHandler );
  sResourceBase = this;

  if ( !QDBusConnection::sessionBus().registerService( QLatin1String( "org.kde.Akonadi.Resource." ) + id ) )
    error( QString::fromLatin1( "Unable to register service at dbus: %1" ).arg( QDBusConnection::sessionBus().lastError().message() ) );

  new ResourceAdaptor( this );
  if ( !QDBusConnection::sessionBus().registerObject( QLatin1String( "/" ), this, QDBusConnection::ExportAdaptors ) )
    error( QString::fromLatin1( "Unable to register object at dbus: %1" ).arg( QDBusConnection::sessionBus().lastError().message() ) );

  const QString name = d->mSettings->value( QLatin1String( "Resource/Name" ) ).toString();
  if ( !name.isEmpty() )
    d->mName = name;

  d->scheduler = new ResourceScheduler( this );

  d->online = settings()->value( QLatin1String( "Resource/Online" ), true ).toBool();
  if ( d->online )
    d->monitor->fetchAllParts();

  d->monitor->setChangeRecordingEnabled( true );
  connect( d->monitor, SIGNAL(changesAdded()),
           d->scheduler, SLOT(scheduleChangeReplay()) );

  d->monitor->monitorResource( d->mId.toLatin1() );

  connect( d->scheduler, SIGNAL(executeFullSync()),
           SLOT(retrieveCollections()) );
  connect( d->scheduler, SIGNAL(executeCollectionSync(Collection)),
           SLOT(slotSynchronizeCollection(Collection)) );
  connect( d->scheduler, SIGNAL(executeItemFetch(DataReference,QStringList,QDBusMessage)),
           SLOT(requestItemDelivery(DataReference,QStringList,QDBusMessage)) );
  connect( d->scheduler, SIGNAL(executeChangeReplay()),
           d->monitor, SLOT(replayNext()) );

  d->scheduler->setOnline( d->online );
  if ( !d->monitor->isEmpty() )
    d->scheduler->scheduleChangeReplay();

  // initial configuration
  bool initialized = settings()->value( QLatin1String( "Resource/Initialized" ), false ).toBool();
  if ( !initialized ) {
    QTimer::singleShot( 0, this, SLOT(configure()) ); // finish construction first
    settings()->setValue( QLatin1String( "Resource/Initialized" ), true );
  }
}

ResourceBase::~ResourceBase()
{
}

int ResourceBase::status() const
{
  return d_func()->mStatusCode;
}

QString ResourceBase::statusMessage() const
{
  return d_func()->mStatusMessage;
}

uint ResourceBase::progress() const
{
  return d_func()->mProgress;
}

QString ResourceBase::progressMessage() const
{
  return d_func()->mProgressMessage;
}

void ResourceBase::changeStatus( Status status, const QString &message )
{
  Q_D( ResourceBase );
  d->mStatusMessage = message;
  d->mStatusCode = 0;

  switch ( status ) {
    case Ready:
      if ( d->mStatusMessage.isEmpty() )
        d->mStatusMessage = d->defaultReadyMessage();

      d->mStatusCode = 0;
      break;
    case Syncing:
      if ( d->mStatusMessage.isEmpty() )
        d->mStatusMessage = d->defaultSyncingMessage();

      d->mStatusCode = 1;
      break;
    case Error:
      if ( d->mStatusMessage.isEmpty() )
        d->mStatusMessage = d->defaultErrorMessage();

      d->mStatusCode = 2;
      break;
    default:
      Q_ASSERT( !"Unknown status passed" );
      break;
  }

  emit statusChanged( d->mStatusCode, d->mStatusMessage );
}

void ResourceBase::changeProgress( uint progress, const QString &message )
{
  Q_D( ResourceBase );
  d->mProgress = progress;
  d->mProgressMessage = message;

  emit progressChanged( d->mProgress, d->mProgressMessage );
}

void ResourceBase::configure()
{
}

void ResourceBase::synchronize()
{
  d_func()->scheduler->scheduleFullSync();
}

void ResourceBase::setName( const QString &name )
{
  Q_D( ResourceBase );
  if ( name == d->mName )
    return;

  // TODO: rename collection
  d->mName = name;

  if ( d->mName.isEmpty() || d->mName == d->mId )
    d->mSettings->remove( QLatin1String( "Resource/Name" ) );
  else
    d->mSettings->setValue( QLatin1String( "Resource/Name" ), d->mName );

  d->mSettings->sync();

  emit nameChanged( d->mName );
}

QString ResourceBase::name() const
{
  const Q_D( ResourceBase );
  if ( d->mName.isEmpty() )
    return d->mId;
  else
    return d->mName;
}

static char* sAppName = 0;

QString ResourceBase::parseArguments( int argc, char **argv )
{
  QString identifier;
  if ( argc < 3 ) {
    qDebug( "ResourceBase::parseArguments: Not enough arguments passed..." );
    exit( 1 );
  }

  for ( int i = 1; i < argc - 1; ++i ) {
    if ( QLatin1String( argv[ i ] ) == QLatin1String( "--identifier" ) )
      identifier = QLatin1String( argv[ i + 1 ] );
  }

  if ( identifier.isEmpty() ) {
    qDebug( "ResourceBase::parseArguments: Identifier argument missing" );
    exit( 1 );
  }

  sAppName = qstrdup( identifier.toLatin1().constData() );
  KCmdLineArgs::init( argc, argv, sAppName, 0,
                      ki18nc("@title, application name", "Akonadi Resource"), "0.1",
                      ki18nc("@title, application description", "Akonadi Resource") );

  KCmdLineOptions options;
  options.add("identifier <argument>",
              ki18nc("@label, commandline option", "Resource identifier"));
  KCmdLineArgs::addCmdLineOptions( options );

  return identifier;
}

int ResourceBase::init( ResourceBase *r )
{
  QApplication::setQuitOnLastWindowClosed( false );
  int rv = kapp->exec();
  delete r;
  delete[] sAppName;
  return rv;
}

void ResourceBase::crashHandler( int signal )
{
  /**
   * If we retrieved a SIGINT or SIGTERM we close normally
   */
  if ( signal == SIGINT || signal == SIGTERM )
    quit();
}

bool ResourceBase::deliverItem(Akonadi::Job * job, const QDBusMessage & msg)
{
  Q_D( ResourceBase );
  msg.setDelayedReply( true );
  d->pendingReplies.insert( job, msg.createReply() );
  connect( job, SIGNAL(result(KJob*)), SLOT(slotDeliveryDone(KJob*)) );
  return false;
}

void ResourceBasePrivate::slotDeliveryDone(KJob * job)
{
  Q_Q( ResourceBase );
  Q_ASSERT( pendingReplies.contains( static_cast<Akonadi::Job*>( job ) ) );
  QDBusMessage reply = pendingReplies.take( static_cast<Akonadi::Job*>( job ) );
  if ( job->error() ) {
    q->error( QLatin1String( "Error while creating item: " ) + job->errorString() );
    reply << false;
  } else {
    reply << true;
  }
  QDBusConnection::sessionBus().send( reply );
  scheduler->taskDone();
}

void ResourceBase::changesCommitted(const DataReference & ref)
{
  Q_D( ResourceBase );
  ItemStoreJob *job = new ItemStoreJob( Item( ref ), session() );
  job->setClean();
  d->monitor->changeProcessed();
  if ( !d->monitor->isEmpty() )
    d->scheduler->scheduleChangeReplay();
  d->scheduler->taskDone();
}

bool ResourceBase::requestItemDelivery(int uid, const QString & remoteId, const QStringList &parts )
{
  Q_D( ResourceBase );
  if ( !isOnline() ) {
    error( i18nc( "@info", "Cannot fetch item in offline mode." ) );
    return false;
  }

  setDelayedReply( true );
  d->scheduler->scheduleItemFetch( DataReference( uid, remoteId ), parts, message() );

  return true;
}

bool ResourceBase::isOnline() const
{
  return d_func()->online;
}

void ResourceBase::setOnline(bool state)
{
  Q_D( ResourceBase );
  d->online = state;
  settings()->setValue( QLatin1String( "Resource/Online" ), state );
  d->monitor->fetchCollection( state );
  // TODO: d->monitor->fetchItemData( state );
  d->scheduler->setOnline( state );
}

void ResourceBase::collectionsRetrieved(const Collection::List & collections)
{
  Q_D( ResourceBase );
  CollectionSync *syncer = new CollectionSync( d->mId, session() );
  syncer->setRemoteCollections( collections );
  connect( syncer, SIGNAL(result(KJob*)), SLOT(slotCollectionSyncDone(KJob*)) );
}

void ResourceBase::collectionsRetrievedIncremental(const Collection::List & changedCollections, const Collection::List & removedCollections)
{
  Q_D( ResourceBase );
  CollectionSync *syncer = new CollectionSync( d->mId, session() );
  syncer->setRemoteCollections( changedCollections, removedCollections );
  connect( syncer, SIGNAL(result(KJob*)), SLOT(slotCollectionSyncDone(KJob*)) );
}

void ResourceBasePrivate::slotCollectionSyncDone(KJob * job)
{
  Q_Q( ResourceBase );
  if ( job->error() ) {
    q->error( job->errorString() );
  } else {
    if ( scheduler->currentTask().type == ResourceScheduler::SyncAll ) {
      CollectionListJob *list = new CollectionListJob( Collection::root(), CollectionListJob::Recursive, session );
      list->setResource( mId );
      q->connect( list, SIGNAL(result(KJob*)), q, SLOT(slotLocalListDone(KJob*)) );
      return;
    }
  }
  q->changeStatus( ResourceBase::Ready );
  scheduler->taskDone();
}

void ResourceBasePrivate::slotLocalListDone(KJob * job)
{
  Q_Q( ResourceBase );
  if ( job->error() ) {
    q->error( job->errorString() );
  } else {
    Collection::List cols = static_cast<CollectionListJob*>( job )->collections();
    foreach ( const Collection &col, cols ) {
      scheduler->scheduleSync( col );
    }
  }
  scheduler->taskDone();
}

void ResourceBasePrivate::slotSynchronizeCollection( const Collection &col )
{
  Q_Q( ResourceBase );
  currentCollection = col;
  // check if this collection actually can contain anything
  QStringList contentTypes = currentCollection.contentTypes();
  contentTypes.removeAll( Collection::collectionMimeType() );
  if ( !contentTypes.isEmpty() ) {
    q->changeStatus( ResourceBase::Syncing, i18nc( "@info:status", "Syncing collection '%1'", currentCollection.name() ) );
    q->synchronizeCollection( currentCollection );
    return;
  }
  scheduler->taskDone();
}

void ResourceBase::collectionSynchronized()
{
  Q_D( ResourceBase );
  changeStatus( Ready );
  d->scheduler->taskDone();
}

Collection ResourceBase::currentCollection() const
{
  return d_func()->currentCollection;
}

void ResourceBase::synchronizeCollection(int collectionId)
{
  CollectionListJob* job = new CollectionListJob( Collection(collectionId), CollectionListJob::Local, session() );
  job->setResource( identifier() );
  connect( job, SIGNAL(result(KJob*)), SLOT(slotCollectionListDone(KJob*)) );
}

void ResourceBasePrivate::slotCollectionListDone( KJob *job )
{
  if ( !job->error() ) {
    Collection::List list = static_cast<CollectionListJob*>( job )->collections();
    if ( !list.isEmpty() ) {
      Collection col = list.first();
      scheduler->scheduleSync( col );
    }
  }
  // TODO: error handling
}

void ResourceBase::itemsRetrieved(const Item::List & items)
{
  ItemSync *syncer = new ItemSync( currentCollection(), session() );
  connect( syncer, SIGNAL(percent(KJob*,unsigned long)), SLOT(slotPercent(KJob*,unsigned long)) );
  connect( syncer, SIGNAL(result(KJob*)), SLOT(slotItemSyncDone(KJob*)) );
  syncer->setRemoteItems( items );
}

void ResourceBase::itemsRetrievedIncremental(const Item::List & changedItems, const Item::List & removedItems)
{
  ItemSync *syncer = new ItemSync( currentCollection(), session() );
  connect( syncer, SIGNAL(percent(KJob*,unsigned long)), SLOT(slotPercent(KJob*,unsigned long)) );
  connect( syncer, SIGNAL(result(KJob*)), SLOT(slotItemSyncDone(KJob*)) );
  syncer->setRemoteItems( changedItems, removedItems );
}

void ResourceBasePrivate::slotItemSyncDone( KJob *job )
{
  Q_Q( ResourceBase );
  if ( job->error() ) {
    q->error( job->errorString() );
  }
  q->changeStatus( ResourceBase::Ready );
  scheduler->taskDone();
}

void ResourceBasePrivate::slotPercent( KJob *job, unsigned long percent )
{
  Q_Q( ResourceBase );
  Q_UNUSED( job );
  q->changeProgress( percent );
}

#include "resource.moc"
#include "resourcebase.moc"
