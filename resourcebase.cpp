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

class ResourceBase::Private
{
  public:
    Private( ResourceBase *parent )
      : mParent( parent ),
        mStatusCode( Ready ),
        mProgress( 0 ),
        mSettings( 0 ),
        online( true ),
        scheduler( new ResourceScheduler( parent ) )
    {
      mStatusMessage = defaultReadyMessage();
    }

    void slotDeliveryDone( KJob* job );

    void slotCollectionSyncDone( KJob *job );
    void slotLocalListDone( KJob *job );
    void slotSynchronizeCollection( const Collection &col );
    void slotCollectionListDone( KJob *job );

    void slotItemSyncDone( KJob *job );

    void slotPercent( KJob* job, unsigned long percent );

    QString defaultReadyMessage() const;
    QString defaultSyncingMessage() const;
    QString defaultErrorMessage() const;

    ResourceBase *mParent;

    org::kde::Akonadi::Tracer *mTracer;
    QString mId;
    QString mName;

    int mStatusCode;
    QString mStatusMessage;

    uint mProgress;
    QString mProgressMessage;

    QSettings *mSettings;

    Session *session;
    ChangeRecorder *monitor;
    QHash<Akonadi::Job*, QDBusMessage> pendingReplies;

    bool online;

    // synchronize states
    Collection currentCollection;

    ResourceScheduler *scheduler;
};

QString ResourceBase::Private::defaultReadyMessage() const
{
  if ( online )
    return i18nc( "@info:status, application ready for work", "Ready" );
  return i18nc( "@info:status", "Offline" );
}

QString ResourceBase::Private::defaultSyncingMessage() const
{
  return i18nc( "@info:status", "Syncing..." );
}

QString ResourceBase::Private::defaultErrorMessage() const
{
  return i18nc( "@info:status", "Error!" );
}

ResourceBase::ResourceBase( const QString & id )
  : d( new Private( this ) )
{
  KCrash::init();
  KCrash::setEmergencyMethod( ::crashHandler );
  sResourceBase = this;

  d->mTracer = new org::kde::Akonadi::Tracer( QLatin1String( "org.kde.Akonadi" ), QLatin1String( "/tracing" ),
                                              QDBusConnection::sessionBus(), this );

  if ( !QDBusConnection::sessionBus().registerService( QLatin1String( "org.kde.Akonadi.Resource." ) + id ) )
    error( QString::fromLatin1( "Unable to register service at dbus: %1" ).arg( QDBusConnection::sessionBus().lastError().message() ) );

  new ResourceAdaptor( this );
  if ( !QDBusConnection::sessionBus().registerObject( QLatin1String( "/" ), this, QDBusConnection::ExportAdaptors ) )
    error( QString::fromLatin1( "Unable to register object at dbus: %1" ).arg( QDBusConnection::sessionBus().lastError().message() ) );

  d->mId = id;

  d->mSettings = new QSettings( QString::fromLatin1( "%1/resource_config_%2" ).arg( XdgBaseDirs::saveDir( "config", QLatin1String( "akonadi" ) ), id ), QSettings::IniFormat );

  const QString name = d->mSettings->value( QLatin1String( "Resource/Name" ) ).toString();
  if ( !name.isEmpty() )
    d->mName = name;

  d->online = settings()->value( QLatin1String( "Resource/Online" ), true ).toBool();

  d->session = new Session( d->mId.toLatin1(), this );
  d->monitor = new ChangeRecorder( this );
  d->monitor->fetchCollection( d->online );
  if ( d->online )
    d->monitor->fetchAllParts();

  connect( d->monitor, SIGNAL( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ),
           SLOT( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ) );
  connect( d->monitor, SIGNAL( itemChanged( const Akonadi::Item&, const QStringList& ) ),
           SLOT( itemChanged( const Akonadi::Item&, const QStringList& ) ) );
  connect( d->monitor, SIGNAL( itemRemoved( const Akonadi::DataReference& ) ),
           SLOT( itemRemoved( const Akonadi::DataReference& ) ) );
  connect( d->monitor, SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)),
           SLOT(collectionAdded(Akonadi::Collection,Akonadi::Collection)) );
  connect( d->monitor, SIGNAL( collectionChanged( const Akonadi::Collection& ) ),
           SLOT( collectionChanged( const Akonadi::Collection& ) ) );
  connect( d->monitor, SIGNAL( collectionRemoved( int, const QString& ) ),
           SLOT( collectionRemoved( int, const QString& ) ) );
  connect( d->monitor, SIGNAL(changesAdded()),
           d->scheduler, SLOT(scheduleChangeReplay()) );

  d->monitor->ignoreSession( session() );
  d->monitor->monitorResource( d->mId.toLatin1() );
  d->monitor->setConfig( settings() );

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
  delete d->mSettings;
  delete d;
}

int ResourceBase::status() const
{
  return d->mStatusCode;
}

QString ResourceBase::statusMessage() const
{
  return d->mStatusMessage;
}

uint ResourceBase::progress() const
{
  return d->mProgress;
}

QString ResourceBase::progressMessage() const
{
  return d->mProgressMessage;
}

void ResourceBase::warning( const QString& message )
{
  d->mTracer->warning( QString::fromLatin1( "ResourceBase(%1)" ).arg( d->mId ), message );
}

void ResourceBase::error( const QString& message )
{
  d->mTracer->error( QString::fromLatin1( "ResourceBase(%1)" ).arg( d->mId ), message );
}

void ResourceBase::changeStatus( Status status, const QString &message )
{
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
  d->mProgress = progress;
  d->mProgressMessage = message;

  emit progressChanged( d->mProgress, d->mProgressMessage );
}

void ResourceBase::configure()
{
}

void ResourceBase::synchronize()
{
  d->scheduler->scheduleFullSync();
}

void ResourceBase::setName( const QString &name )
{
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

void ResourceBase::quit()
{
  aboutToQuit();

  d->mSettings->sync();

  QTimer::singleShot( 0, QCoreApplication::instance(), SLOT( quit() ) );
}

void ResourceBase::aboutToQuit()
{
}

QString ResourceBase::identifier() const
{
  return d->mId;
}

void ResourceBase::cleanup() const
{
  const QString fileName = d->mSettings->fileName();

  /**
   * First destroy the settings object...
   */
  delete d->mSettings;
  d->mSettings = 0;

  /**
   * ... then remove the file from hd.
   */
  QFile::remove( fileName );

  QCoreApplication::exit( 0 );
}

void ResourceBase::crashHandler( int signal )
{
  /**
   * If we retrieved a SIGINT or SIGTERM we close normally
   */
  if ( signal == SIGINT || signal == SIGTERM )
    quit();
}

QSettings* ResourceBase::settings()
{
  return d->mSettings;
}

Session* ResourceBase::session()
{
  return d->session;
}

bool ResourceBase::deliverItem(Akonadi::Job * job, const QDBusMessage & msg)
{
  msg.setDelayedReply( true );
  d->pendingReplies.insert( job, msg.createReply() );
  connect( job, SIGNAL(result(KJob*)), SLOT(slotDeliveryDone(KJob*)) );
  return false;
}

void ResourceBase::itemAdded( const Item &item, const Collection &collection )
{
  Q_UNUSED( item );
  Q_UNUSED( collection );
}

void ResourceBase::itemChanged( const Item &item, const QStringList &partIdentifiers )
{
  Q_UNUSED( item );
  Q_UNUSED( partIdentifiers );
}

void ResourceBase::itemRemoved( const DataReference &ref )
{
  Q_UNUSED( ref );
}

void ResourceBase::collectionAdded( const Collection &collection, const Collection &parent )
{
  Q_UNUSED( collection );
  Q_UNUSED( parent );
}

void ResourceBase::collectionChanged( const Collection &collection )
{
  Q_UNUSED( collection );
}

void ResourceBase::collectionRemoved( int id, const QString &remoteId )
{
  Q_UNUSED( id );
  Q_UNUSED( remoteId );
}

void ResourceBase::Private::slotDeliveryDone(KJob * job)
{
  Q_ASSERT( pendingReplies.contains( static_cast<Akonadi::Job*>( job ) ) );
  QDBusMessage reply = pendingReplies.take( static_cast<Akonadi::Job*>( job ) );
  if ( job->error() ) {
    mParent->error( QLatin1String( "Error while creating item: " ) + job->errorString() );
    reply << false;
  } else {
    reply << true;
  }
  QDBusConnection::sessionBus().send( reply );
  scheduler->taskDone();
}

void ResourceBase::changesCommitted(const DataReference & ref)
{
  ItemStoreJob *job = new ItemStoreJob( Item( ref ), session() );
  job->setClean();
  d->monitor->changeProcessed();
  if ( !d->monitor->isEmpty() )
    d->scheduler->scheduleChangeReplay();
  d->scheduler->taskDone();
}

bool ResourceBase::requestItemDelivery(int uid, const QString & remoteId, const QStringList &parts )
{
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
  return d->online;
}

void ResourceBase::setOnline(bool state)
{
  d->online = state;
  settings()->setValue( QLatin1String( "Resource/Online" ), state );
  d->monitor->fetchCollection( state );
  // TODO: d->monitor->fetchItemData( state );
  d->scheduler->setOnline( state );
}

void ResourceBase::collectionsRetrieved(const Collection::List & collections)
{
  CollectionSync *syncer = new CollectionSync( d->mId, session() );
  syncer->setRemoteCollections( collections );
  connect( syncer, SIGNAL(result(KJob*)), SLOT(slotCollectionSyncDone(KJob*)) );
}

void ResourceBase::collectionsRetrievedIncremental(const Collection::List & changedCollections, const Collection::List & removedCollections)
{
  CollectionSync *syncer = new CollectionSync( d->mId, session() );
  syncer->setRemoteCollections( changedCollections, removedCollections );
  connect( syncer, SIGNAL(result(KJob*)), SLOT(slotCollectionSyncDone(KJob*)) );
}

void ResourceBase::Private::slotCollectionSyncDone(KJob * job)
{
  if ( job->error() ) {
    mParent->error( job->errorString() );
  } else {
    if ( scheduler->currentTask().type == ResourceScheduler::SyncAll ) {
      CollectionListJob *list = new CollectionListJob( Collection::root(), CollectionListJob::Recursive, mParent->session() );
      list->setResource( mId );
      mParent->connect( list, SIGNAL(result(KJob*)), mParent, SLOT(slotLocalListDone(KJob*)) );
      return;
    }
  }
  mParent->changeStatus( Ready );
  scheduler->taskDone();
}

void ResourceBase::Private::slotLocalListDone(KJob * job)
{
  if ( job->error() ) {
    mParent->error( job->errorString() );
  } else {
    Collection::List cols = static_cast<CollectionListJob*>( job )->collections();
    foreach ( const Collection &col, cols ) {
      scheduler->scheduleSync( col );
    }
  }
  scheduler->taskDone();
}

void ResourceBase::Private::slotSynchronizeCollection( const Collection &col )
{
  currentCollection = col;
  // check if this collection actually can contain anything
  QStringList contentTypes = currentCollection.contentTypes();
  contentTypes.removeAll( Collection::collectionMimeType() );
  if ( !contentTypes.isEmpty() ) {
    mParent->changeStatus( Syncing, i18nc( "@info:status", "Syncing collection '%1'", currentCollection.name() ) );
    mParent->synchronizeCollection( currentCollection );
    return;
  }
  scheduler->taskDone();
}

void ResourceBase::collectionSynchronized()
{
  changeStatus( Ready );
  d->scheduler->taskDone();
}

Collection ResourceBase::currentCollection() const
{
  return d->currentCollection;
}

void ResourceBase::synchronizeCollection(int collectionId)
{
  CollectionListJob* job = new CollectionListJob( Collection(collectionId), CollectionListJob::Local, session() );
  job->setResource( identifier() );
  connect( job, SIGNAL(result(KJob*)), SLOT(slotCollectionListDone(KJob*)) );
}

void ResourceBase::Private::slotCollectionListDone( KJob *job )
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

void ResourceBase::Private::slotItemSyncDone( KJob *job )
{
  if ( job->error() ) {
    mParent->error( job->errorString() );
  }
  mParent->changeStatus( Ready );
  scheduler->taskDone();
}

void ResourceBase::Private::slotPercent( KJob *job, unsigned long percent )
{
  Q_UNUSED( job );
  mParent->changeProgress( percent );
}

#include "resource.moc"
#include "resourcebase.moc"
