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
#include "monitor_p.h"
#include "resourcescheduler.h"
#include "tracerinterface.h"

#include <libakonadi/collectionlistjob.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/job.h>
#include <libakonadi/session.h>
#include <libakonadi/monitor.h>

#include <kaboutdata.h>
#include <kcmdlineargs.h>
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
        changeRecording( false ),
        scheduler( new ResourceScheduler( parent ) )
    {
      mStatusMessage = defaultReadyMessage();
    }

    void slotDeliveryDone( KJob* job );

    void slotItemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void slotItemChanged( const Akonadi::Item &item, const QStringList& );
    void slotItemRemoved( const Akonadi::DataReference &reference );
    void slotCollectionAdded( const Akonadi::Collection &collection );
    void slotCollectionChanged( const Akonadi::Collection &collection );
    void slotCollectionRemoved( int id, const QString &remoteId );

    void slotReplayNextItem();
    void slotReplayItemAdded( KJob *job );
    void slotReplayItemChanged( KJob *job );
    void slotReplayCollectionAdded( KJob *job );
    void slotReplayCollectionChanged( KJob *job );

    void slotCollectionSyncDone( KJob *job );
    void slotLocalListDone( KJob *job );
    void slotSynchronizeCollection( const Collection &col );
    void slotCollectionListDone( KJob *job );

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
    Monitor *monitor;
    QHash<Akonadi::Job*, QDBusMessage> pendingReplys;

    bool online;

    // change recording
    enum ChangeType {
      ItemAdded, ItemChanged, ItemRemoved, CollectionAdded, CollectionChanged, CollectionRemoved
    };
    struct ChangeItem {
      ChangeType type;
      DataReference item;
      int collectionId;
      QString collectionRemoteId;
      QStringList partIdentifiers;
    };
    bool changeRecording;
    QList<ChangeItem> changes;

    void addChange( ChangeItem &c )
    {
      bool skipCurrent = false;
      // compress changes
      for ( QList<ChangeItem>::Iterator it = changes.begin(); it != changes.end(); ) {
        if ( !(*it).item.isNull() && (*it).item == c.item ) {
          if ( (*it).type == ItemAdded && c.type == ItemRemoved )
            skipCurrent = true; // add + remove -> noop
          else if ( (*it).type == ItemAdded && c.type == ItemChanged )
            c.type = ItemAdded; // add + change -> add
          it = changes.erase( it );
        } else if ( (*it).collectionId >= 0 && (*it).collectionId == c.collectionId ) {
          if ( (*it).type == CollectionAdded && c.type == CollectionRemoved )
            skipCurrent = true; // add + remove -> noop
          else if ( (*it).type == CollectionAdded && c.type == CollectionChanged )
            c.type = ItemAdded; // add + change -> add
          it = changes.erase( it );
        } else
          ++it;
      }

      if ( !skipCurrent )
        changes << c;
    }

    void loadChanges()
    {
      changes.clear();
      mSettings->beginGroup( QLatin1String( "Changes" ) );
      int size = mSettings->beginReadArray( QLatin1String( "change" ) );
      for ( int i = 0; i < size; ++i ) {
        mSettings->setArrayIndex( i );
        ChangeItem c;
        c.type = (ChangeType)mSettings->value( QLatin1String( "type" ) ).toInt();
        c.item = DataReference( mSettings->value( QLatin1String( "item_uid" ) ).toInt(),
                                mSettings->value( QLatin1String( "item_rid" ) ).toString() );
        c.collectionId = mSettings->value( QLatin1String( "collectionId" ) ).toInt();
        c.collectionRemoteId = mSettings->value( QLatin1String( "collectionRemoteId" ) ).toString();
        c.partIdentifiers = mSettings->value( QLatin1String( "partIdentifiers" ) ).toStringList();
        changes << c;
      }
      mSettings->endArray();
      mSettings->endGroup();
      changeRecording = mSettings->value( QLatin1String( "Resource/ChangeRecording" ), false ).toBool();
    }

    void saveChanges()
    {
      mSettings->beginGroup( QLatin1String( "Changes" ) );
      mSettings->beginWriteArray( QLatin1String( "change" ), changes.count() );
      for ( int i = 0; i < changes.count(); ++i ) {
        mSettings->setArrayIndex( i );
        ChangeItem c = changes.at( i );
        mSettings->setValue( QLatin1String( "type" ), c.type );
        mSettings->setValue( QLatin1String( "item_uid" ), c.item.id() );
        mSettings->setValue( QLatin1String( "item_rid" ), c.item.remoteId() );
        mSettings->setValue( QLatin1String( "collectionId" ), c.collectionId );
        mSettings->setValue( QLatin1String( "collectionRemoteId" ), c.collectionRemoteId );
        mSettings->setValue( QLatin1String( "partIdentifiers" ), c.partIdentifiers );
      }
      mSettings->endArray();
      mSettings->endGroup();
      mSettings->setValue( QLatin1String( "Resource/ChangeRecording" ), changeRecording );
    }

    // synchronize states
    Collection currentCollection;

    ResourceScheduler *scheduler;
};

QString ResourceBase::Private::defaultReadyMessage() const
{
  if ( online )
    return i18n( "Ready" );
  return i18n( "Offline" );
}

QString ResourceBase::Private::defaultSyncingMessage() const
{
  return tr( "Syncing..." );
}

QString ResourceBase::Private::defaultErrorMessage() const
{
  return tr( "Error!" );
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

  d->mSettings = new QSettings( QString::fromLatin1( "%1/.akonadi/resource_config_%2" ).arg( QDir::homePath(), id ), QSettings::IniFormat );

  const QString name = d->mSettings->value( QLatin1String( "Resource/Name" ) ).toString();
  if ( !name.isEmpty() )
    d->mName = name;

  d->online = settings()->value( QLatin1String( "Resource/Online" ), true ).toBool();

  d->session = new Session( d->mId.toLatin1(), this );
  d->monitor = new Monitor( this );
  d->monitor->fetchCollection( d->online );
  if ( d->online )
    d->monitor->addFetchPart( Item::PartAll );

  connect( d->monitor, SIGNAL( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ),
           this, SLOT( slotItemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ) );
  connect( d->monitor, SIGNAL( itemChanged( const Akonadi::Item&, const QStringList& ) ),
           this, SLOT( slotItemChanged( const Akonadi::Item&, const QStringList& ) ) );
  connect( d->monitor, SIGNAL( itemRemoved( const Akonadi::DataReference& ) ),
           this, SLOT( slotItemRemoved( const Akonadi::DataReference& ) ) );
  connect( d->monitor, SIGNAL( collectionAdded( const Akonadi::Collection& ) ),
           this, SLOT( slotCollectionAdded( const Akonadi::Collection& ) ) );
  connect( d->monitor, SIGNAL( collectionChanged( const Akonadi::Collection& ) ),
           this, SLOT( slotCollectionChanged( const Akonadi::Collection& ) ) );
  connect( d->monitor, SIGNAL( collectionRemoved( int, const QString& ) ),
           this, SLOT( slotCollectionRemoved( int, const QString& ) ) );

  d->monitor->ignoreSession( session() );
  d->monitor->monitorResource( d->mId.toLatin1() );

  connect( d->scheduler, SIGNAL(executeFullSync()),
           SLOT(retrieveCollections()) );
  connect( d->scheduler, SIGNAL(executeCollectionSync(Collection)),
           SLOT(slotSynchronizeCollection(Collection)) );
  connect( d->scheduler, SIGNAL(executeItemFetch(DataReference,QStringList,QDBusMessage)),
           SLOT(requestItemDelivery(DataReference,QStringList,QDBusMessage)) );

  d->loadChanges();

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
  KCmdLineArgs::init( argc, argv, sAppName, 0, ki18n("Akonadi Resource"),"0.1" ,
                      ki18n("Akonadi Resource") );

  KCmdLineOptions options;
  options.add("identifier <argument>", ki18n("Resource identifier"));
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
  d->saveChanges();
  aboutToQuit();

  d->mSettings->sync();

  QCoreApplication::exit( 0 );
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
  d->pendingReplys.insert( job, msg.createReply() );
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

void ResourceBase::Private::slotDeliveryDone(KJob * job)
{
  Q_ASSERT( pendingReplys.contains( static_cast<Akonadi::Job*>( job ) ) );
  QDBusMessage reply = pendingReplys.take( static_cast<Akonadi::Job*>( job ) );
  if ( job->error() ) {
    mParent->error( QLatin1String( "Error while creating item: " ) + job->errorString() );
    reply << false;
  } else {
    reply << true;
  }
  QDBusConnection::sessionBus().send( reply );
  scheduler->taskDone();
}

void ResourceBase::enableChangeRecording(bool enable)
{
  if ( d->changeRecording == enable )
    return;
  d->changeRecording = enable;
  if ( !d->changeRecording ) {
    d->slotReplayNextItem();
  }
}

void ResourceBase::Private::slotReplayNextItem()
{
  if ( changes.count() > 0 ) {
    const Private::ChangeItem c = changes.takeFirst();
    switch ( c.type ) {
      case Private::ItemAdded:
        {
          ItemCollectionFetchJob *job = new ItemCollectionFetchJob( c.item, c.collectionId, mParent );
          mParent->connect( job, SIGNAL( result( KJob* ) ), mParent, SLOT( slotReplayItemAdded( KJob* ) ) );
          job->start();
        }
        break;
      case Private::ItemChanged:
        {
          ItemFetchJob *job = new ItemFetchJob( c.item, mParent );
          foreach( QString part, c.partIdentifiers )
            job->addFetchPart( part );
          job->setProperty( "parts", QVariant( c.partIdentifiers ) );
          mParent->connect( job, SIGNAL( result( KJob* ) ), mParent, SLOT( slotReplayItemChanged( KJob* ) ) );
          job->start();
        }
        break;
      case Private::ItemRemoved:
        mParent->itemRemoved( c.item );
        break;
      case Private::CollectionAdded:
        {
          CollectionListJob *job = new CollectionListJob( Collection( c.collectionId), CollectionListJob::Flat, mParent );
          mParent->connect( job, SIGNAL( result( KJob* ) ), mParent, SLOT( slotReplayCollectionAdded( KJob* ) ) );
          job->start();
        }
        break;
      case Private::CollectionChanged:
        {
          CollectionListJob *job = new CollectionListJob( Collection( c.collectionId), CollectionListJob::Flat, mParent );
          mParent->connect( job, SIGNAL( result( KJob* ) ), mParent, SLOT( slotReplayCollectionChanged( KJob* ) ) );
          job->start();
        }
        break;
      case Private::CollectionRemoved:
        mParent->collectionRemoved( c.collectionId, c.collectionRemoteId );
        break;
    }
  }
}

void ResourceBase::Private::slotReplayItemAdded( KJob *job )
{
  if ( job->error() ) {
    mParent->error( i18n( "Unable to fetch item in replay mode." ) );
  } else {
    ItemCollectionFetchJob *fetchJob = qobject_cast<ItemCollectionFetchJob*>( job );

    const Item item = fetchJob->item();
    const Collection collection = fetchJob->collection();
    if ( item.isValid() )
      mParent->itemAdded( item, collection );
  }

  QTimer::singleShot( 0, mParent, SLOT( slotReplayNextItem() ) );
}

void ResourceBase::Private::slotReplayItemChanged( KJob *job )
{
  if ( job->error() ) {
    mParent->error( i18n( "Unable to fetch item in replay mode." ) );
  } else {
    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>( job );

    const Item item = fetchJob->items().first();
    if ( item.isValid() ) {
      const QStringList parts = job->property( "parts" ).toStringList();
      mParent->itemChanged( item, parts );
    }
  }

  QTimer::singleShot( 0, mParent, SLOT( slotReplayNextItem() ) );
}

void ResourceBase::Private::slotReplayCollectionAdded( KJob *job )
{
  if ( job->error() ) {
    mParent->error( i18n( "Unable to fetch collection in replay mode." ) );
  } else {
    CollectionListJob *listJob = qobject_cast<CollectionListJob*>( job );

    if ( listJob->collections().count() == 0 )
      mParent->error( i18n( "Unable to fetch collection in replay mode." ) );
    else
      mParent->collectionAdded( listJob->collections().first() );
  }

  QTimer::singleShot( 0, mParent, SLOT( slotReplayNextItem() ) );
}

void ResourceBase::Private::slotReplayCollectionChanged( KJob *job )
{
  if ( job->error() ) {
    mParent->error( i18n( "Unable to fetch collection in replay mode." ) );
  } else {
    CollectionListJob *listJob = qobject_cast<CollectionListJob*>( job );

    if ( listJob->collections().count() == 0 )
      mParent->error( i18n( "Unable to fetch collection in replay mode." ) );
    else
      mParent->collectionChanged( listJob->collections().first() );
  }

  QTimer::singleShot( 0, mParent, SLOT( slotReplayNextItem() ) );
}

void ResourceBase::Private::slotItemAdded( const Akonadi::Item &item, const Collection &collection )
{
  if ( changeRecording ) {
    Private::ChangeItem c;
    c.type = Private::ItemAdded;
    c.item = item.reference();
    c.collectionId = collection.id();
    addChange( c );
  } else {
    mParent->itemAdded( item, collection );
  }
}

void ResourceBase::Private::slotItemChanged( const Akonadi::Item &item, const QStringList &partIdentifiers )
{
  if ( changeRecording ) {
    Private::ChangeItem c;
    c.type = Private::ItemChanged;
    c.item = item.reference();
    c.collectionId = -1;
    c.partIdentifiers = partIdentifiers;
    addChange( c );
  } else {
    mParent->itemChanged( item, partIdentifiers );
  }
}

void ResourceBase::Private::slotItemRemoved(const Akonadi::DataReference & ref)
{
  if ( changeRecording ) {
    Private::ChangeItem c;
    c.type = Private::ItemRemoved;
    c.item = ref;
    c.collectionId = -1;
    addChange( c );
  } else {
    mParent->itemRemoved( ref );
  }
}

void ResourceBase::Private::slotCollectionAdded( const Akonadi::Collection &collection )
{
  if ( changeRecording ) {
    Private::ChangeItem c;
    c.type = Private::CollectionAdded;
    c.collectionId = collection.id();
    addChange( c );
  } else {
    mParent->collectionAdded( collection );
  }
}

void ResourceBase::Private::slotCollectionChanged( const Akonadi::Collection &collection )
{
  if ( changeRecording ) {
    Private::ChangeItem c;
    c.type = Private::CollectionChanged;
    c.collectionId = collection.id();
    addChange( c );
  } else {
    mParent->collectionChanged( collection );
  }
}

void ResourceBase::Private::slotCollectionRemoved( int id, const QString &remoteId )
{
  if ( changeRecording ) {
    Private::ChangeItem c;
    c.type = Private::CollectionRemoved;
    c.collectionId = id;
    c.collectionRemoteId = remoteId;
    addChange( c );
  } else {
    mParent->collectionRemoved( id, remoteId );
  }
}

void ResourceBase::changesCommitted(const DataReference & ref)
{
  ItemStoreJob *job = new ItemStoreJob( ref, session() );
  job->setClean();
}

bool ResourceBase::requestItemDelivery(int uid, const QString & remoteId, const QStringList &parts )
{
  if ( !isOnline() ) {
    error( i18n( "Cannot fetch item in offline mode." ) );
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
  enableChangeRecording( !state );
  d->monitor->fetchCollection( state );
  // TODO: d->monitor->fetchItemData( state );
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
    mParent->changeStatus( Syncing, i18n( "Syncing collection '%1'", currentCollection.name() ) );
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


#include "resource.moc"
#include "resourcebase.moc"
