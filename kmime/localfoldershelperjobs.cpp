/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "localfoldershelperjobs_p.h"

#include "localfolderattribute.h"
#include "localfolders.h"
#include "localfolderssettings.h"

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QTime>
#include <QTimer>
#include <QVariant>

#include <KDebug>
#include <KLocalizedString>
#include <KStandardDirs>

#include <akonadi/agentinstance.h>
#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/agentmanager.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/resourcesynchronizationjob.h>

#define DBUS_SERVICE_NAME QLatin1String( "org.kde.pim.LocalFolders" )
#define LOCK_WAIT_TIMEOUT_SECONDS 3

using namespace Akonadi;

typedef LocalFoldersSettings Settings;


// ===================== ResourceScanJob ============================

/**
  @internal
*/
class Akonadi::ResourceScanJob::Private
{
  public:
    Private( ResourceScanJob *qq );

    void fetchResult( KJob *job ); // slot

    ResourceScanJob *const q;

    // Input:
    QString resourceId;

    // Output:
    Collection rootCollection;
    Collection::List localFolders;
};

ResourceScanJob::Private::Private( ResourceScanJob *qq )
  : q( qq )
{
}

void ResourceScanJob::Private::fetchResult( KJob *job )
{
  if( job->error() ) {
    return;
  }

  Q_ASSERT( dynamic_cast<CollectionFetchJob*>( job ) );
  CollectionFetchJob *fjob = static_cast<CollectionFetchJob *>( job );

  Q_ASSERT( !rootCollection.isValid() );
  Q_ASSERT( localFolders.isEmpty() );
  foreach( const Collection &col, fjob->collections() ) {
    if( col.parentCollection() == Collection::root() ) {
      if( rootCollection.isValid() ) {
        kWarning() << "Resource has more than one collection. I don't know what to do.";
      } else {
        rootCollection = col;
      }
    }

    if( col.hasAttribute<LocalFolderAttribute>() ) {
      localFolders.append( col );
    }
  }

  kDebug() << "Fetched root collection" << rootCollection.id()
           << "and" << localFolders.count() << "local folders"
           << "(total" << fjob->collections().count() << "collections).";

  if( !rootCollection.isValid() ) {
    q->setError( Unknown );
    q->setErrorText( i18n( "Could not fetch root collection of resource %1.", resourceId ) );
    q->commit();
    return;
  }

  // We are done!
  q->commit();
}



ResourceScanJob::ResourceScanJob( const QString &resourceId, QObject *parent )
  : TransactionSequence( parent )
  , d( new Private( this ) )
{
  setResourceId( resourceId );
}

ResourceScanJob::~ResourceScanJob()
{
  delete d;
}

QString ResourceScanJob::resourceId() const
{
  return d->resourceId;
}

void ResourceScanJob::setResourceId( const QString &resourceId )
{
  d->resourceId = resourceId;
}

Collection ResourceScanJob::rootResourceCollection() const
{
  return d->rootCollection;
}

Collection::List ResourceScanJob::localFolders() const
{
  return d->localFolders;
}

void ResourceScanJob::doStart()
{
  if( d->resourceId.isEmpty() ) {
    kError() << "No resource ID given.";
    setError( Job::Unknown );
    setErrorText( i18n( "No resource ID given." ) );
    emitResult();
    TransactionSequence::doStart(); // HACK: probable misuse of TransactionSequence API.
                                    // Calling commit() here hangs :-/
    return;
  }

  CollectionFetchJob *fjob = new CollectionFetchJob( Collection::root(),
                                     CollectionFetchJob::Recursive, this );
  fjob->fetchScope().setResource( d->resourceId );
  connect( fjob, SIGNAL(result(KJob*)), this, SLOT(fetchResult(KJob*)) );
}


// ===================== DefaultResourceJob ============================

/**
  @internal
*/
class Akonadi::DefaultResourceJob::Private
{
  public:
    Private( DefaultResourceJob *qq );

    void tryFetchResource();
    void resourceCreateResult( KJob *job ); // slot
    void resourceSyncResult( KJob *job ); // slot
    void collectionFetchResult( KJob *job ); // slot
    void collectionModifyResult( KJob *job ); // slot

    DefaultResourceJob *const q;
    bool resourceWasPreexisting;
    int pendingModifyJobs;
};

DefaultResourceJob::Private::Private( DefaultResourceJob *qq )
  : q( qq )
  , resourceWasPreexisting( true /* for safety, so as not to accidentally delete data */ )
  , pendingModifyJobs( 0 )
{
}

void DefaultResourceJob::Private::tryFetchResource()
{
  // Get the resourceId from config. Another instance might have changed it in the meantime.
  Settings::self()->readConfig();
  kDebug() << "Read defaultResourceId" << Settings::defaultResourceId() << "from config.";

  AgentInstance resource = AgentManager::self()->instance( Settings::defaultResourceId() );
  if( resource.isValid() ) {
    // The resource exists; scan it.
    resourceWasPreexisting = true;
    kDebug() << "Found resource" << Settings::defaultResourceId();
    q->setResourceId( Settings::defaultResourceId() );
    q->ResourceScanJob::doStart();
  } else {
    // Create the resource.
    resourceWasPreexisting = false;
    kDebug() << "Creating maildir resource.";
    AgentType type = AgentManager::self()->type( QString::fromLatin1( "akonadi_maildir_resource" ) );
    AgentInstanceCreateJob *job = new AgentInstanceCreateJob( type, q );
    connect( job, SIGNAL(result(KJob*)), q, SLOT(resourceCreateResult(KJob*)) );
    job->start(); // non-Akonadi::Job
  }
}

void DefaultResourceJob::Private::resourceCreateResult( KJob *job )
{
  if( job->error() ) {
    //fail( i18n( "Failed to create the default resource (%1).", job->errorString() ) );
    return;
  }

  AgentInstance agent;

  // Get the resource instance.
  {
    Q_ASSERT( dynamic_cast<AgentInstanceCreateJob*>( job ) );
    AgentInstanceCreateJob *cjob = static_cast<AgentInstanceCreateJob*>( job );
    agent = cjob->instance();
    Settings::setDefaultResourceId( agent.identifier() );
    kDebug() << "Created maildir resource with id" << Settings::defaultResourceId();
  }

  // Configure the resource.
  {
    agent.setName( displayNameForType( LocalFolders::Root ) );
    QDBusInterface conf( QString::fromLatin1( "org.freedesktop.Akonadi.Resource." ) + Settings::defaultResourceId(),
                         QString::fromLatin1( "/Settings" ),
                         QString::fromLatin1( "org.kde.Akonadi.Maildir.Settings" ) );
    QDBusReply<void> reply = conf.call( QString::fromLatin1( "setPath" ),
                                        KGlobal::dirs()->localxdgdatadir() + nameForType( LocalFolders::Root ) );
    if( !reply.isValid() ) {
      q->setError( Job::Unknown );
      q->setErrorText( i18n( "Failed to set the root maildir via D-Bus." ) );
      q->commit();
      return;
    } else {
      agent.reconfigure();
    }
  }

  // Sync the resource.
  {
    ResourceSynchronizationJob *sjob = new ResourceSynchronizationJob( agent, q );
    QObject::connect( sjob, SIGNAL(result(KJob*)), q, SLOT(resourceSyncResult(KJob*)) );
    sjob->start(); // non-Akonadi
  }
}

void DefaultResourceJob::Private::resourceSyncResult( KJob *job )
{
  if( job->error() ) {
    //fail( i18n( "ResourceSynchronizationJob failed (%1).", job->errorString() ) );
    return;
  }

  // Fetch the collections of the resource.
  kDebug() << "Fetching maildir collections.";
  CollectionFetchJob *fjob = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive, q );
  fjob->fetchScope().setResource( Settings::defaultResourceId() );
  QObject::connect( fjob, SIGNAL(result(KJob*)), q, SLOT(collectionFetchResult(KJob*)) );
}

void DefaultResourceJob::Private::collectionFetchResult( KJob *job )
{
  if( job->error() ) {
    //fail( i18n( "Failed to fetch the root maildir collection (%1).", job->errorString() ) );
    return;
  }

  Q_ASSERT( dynamic_cast<CollectionFetchJob*>( job ) );
  CollectionFetchJob *fjob = static_cast<CollectionFetchJob *>( job );
  const Collection::List cols = fjob->collections();
  kDebug() << "Fetched" << cols.count() << "collections.";

  // Find the root maildir collection.
  Collection::List toRecover;
  Collection maildirRoot;
  foreach( const Collection &col, cols ) {
    if( col.parentCollection() == Collection::root() ) {
      maildirRoot = col;
      toRecover.append( col );
      break;
    }
  }
  if( !maildirRoot.isValid() ) {
    q->setError( Job::Unknown );
    q->setErrorText( i18n( "Failed to fetch the root maildir collection." ) );
    q->commit();
    return;
  }

  // Find all children of the root collection.
  foreach( const Collection &col, cols ) {
    if( col.parentCollection() == maildirRoot ) {
      toRecover.append( col );
    }
  }

  QHash<QString, int> typeForName;
  for( int type = 0; type < LocalFolders::LastDefaultType; type++ ) {
    typeForName[ nameForType( type ) ] = type;
  }

  // These collections have been created by the maildir resource, when it
  // found the folders on disk. So give them the necessary attributes now.
  Q_ASSERT( pendingModifyJobs == 0 );
  foreach( Collection col, toRecover ) {          // krazy:exclude=foreach

    // Find the type for the collection. For the root collection, we can't use typeForName(), as
    // the root collection name is i18n'ed, as it comes from the resource name
    int type = -1;
    if ( typeForName.contains( col.name() ) )
      type = typeForName[ col.name() ];
    else if ( col == maildirRoot )
      type = LocalFolders::Root;

    if( type != -1 ) {
      kDebug() << "Recovering collection" << col.name();
      setCollectionAttributes( col, type );
      CollectionModifyJob *mjob = new CollectionModifyJob( col, q );
      QObject::connect( mjob, SIGNAL(result(KJob*)), q, SLOT(collectionModifyResult(KJob*)) );
      pendingModifyJobs++;
    } else {
      kDebug() << "Unknown collection name" << col.name() << "-- not recovering.";
    }
  }
  Q_ASSERT( pendingModifyJobs > 0 ); // At least Root.
}

void DefaultResourceJob::Private::collectionModifyResult( KJob *job )
{
  if( job->error() ) {
    //fail( i18n( "Failed to modify the root maildir collection (%1).", job->errorString() ) );
    return;
  }

  Q_ASSERT( pendingModifyJobs > 0 );
  pendingModifyJobs--;
  kDebug() << "pendingModifyJobs now" << pendingModifyJobs;
  if( pendingModifyJobs == 0 ) {
    // Write the updated config.
    kDebug() << "Writing defaultResourceId" << Settings::defaultResourceId() << "to config.";
    Settings::self()->writeConfig();

    // Scan the resource.
    q->setResourceId( Settings::defaultResourceId() );
    q->ResourceScanJob::doStart();
  }
}



DefaultResourceJob::DefaultResourceJob( QObject *parent )
  : ResourceScanJob( QString(), parent )
  , d( new Private( this ) )
{
}

DefaultResourceJob::~DefaultResourceJob()
{
  delete d;
}

void DefaultResourceJob::doStart()
{
  d->tryFetchResource();
}

void DefaultResourceJob::slotResult( KJob *job )
{
  if( job->error() ) {
    // Do some cleanup.
    if( !d->resourceWasPreexisting ) {
      // We only removed the resource instance if we have created it.
      // Otherwise we might lose the user's data.
      const AgentInstance resource = AgentManager::self()->instance( Settings::defaultResourceId() );
      kDebug() << "Removing resource" << resource.identifier();
      AgentManager::self()->removeInstance( resource );
    }
  }

  TransactionSequence::slotResult( job );
}

// ===================== GetLockJob ============================

class Akonadi::GetLockJob::Private
{
  public:
    Private( GetLockJob *qq );

    void doStart(); // slot
    void serviceOwnerChanged( const QString &name, const QString &oldOwner,
                              const QString &newOwner ); // slot
    void timeout(); // slot

    GetLockJob *const q;
    QTimer *safetyTimer;
    QTime timeElapsed;
};

GetLockJob::Private::Private( GetLockJob *qq )
  : q( qq )
  , safetyTimer( 0 )
{
}

void GetLockJob::Private::doStart()
{
  // Just doing registerService() and checking its return value is not sufficient,
  // since we may *already* own the name, and then registerService() returns true.

  QDBusConnection bus = QDBusConnection::sessionBus();
  const bool alreadyLocked = bus.interface()->isServiceRegistered( DBUS_SERVICE_NAME );
  const bool gotIt = bus.registerService( DBUS_SERVICE_NAME );

  if( gotIt && !alreadyLocked ) {
    //kDebug() << "Got lock immediately.";
    q->emitResult();
  } else {
    //kDebug() << "Waiting for lock.";
    connect( QDBusConnection::sessionBus().interface(),
             SIGNAL(serviceOwnerChanged(QString,QString,QString)),
             q, SLOT(serviceOwnerChanged(QString,QString,QString)) );

    safetyTimer = new QTimer( q );
    safetyTimer->setSingleShot( true );
    safetyTimer->setInterval( LOCK_WAIT_TIMEOUT_SECONDS * 1000 );
    safetyTimer->start();
    connect( safetyTimer, SIGNAL(timeout()), q, SLOT(timeout()) );
    timeElapsed.restart();
  }
}

void GetLockJob::Private::serviceOwnerChanged( const QString &name, const QString &oldOwner,
                                      const QString &newOwner )
{
  Q_UNUSED( oldOwner );

  if( name == DBUS_SERVICE_NAME && newOwner.isEmpty() ) {
    const bool gotIt = QDBusConnection::sessionBus().registerService( DBUS_SERVICE_NAME );
    if( gotIt ) {
      safetyTimer->stop();
      //kDebug() << "Got lock after" << timeElapsed.elapsed() << "ms.";
      q->emitResult();
    } else {
      //kDebug() << "Someone got the lock before me.";
    }
  }
}

void GetLockJob::Private::timeout()
{
  kWarning() << "Timeout trying to get lock.";
  q->setError( Job::Unknown );
  q->setErrorText( i18n( "Timeout trying to get lock." ) );
  q->emitResult();
}


GetLockJob::GetLockJob( QObject *parent )
  : KJob( parent )
  , d( new Private( this ) )
{
}

GetLockJob::~GetLockJob()
{
  delete d;
}

void GetLockJob::start()
{
  QTimer::singleShot( 0, this, SLOT(doStart()) );
}


// ===================== helper functions ============================

QString Akonadi::nameForType( int type )
{
  switch( type ) {
    case LocalFolders::Root: return QLatin1String( "local-mail" );
    case LocalFolders::Inbox: return QLatin1String( "inbox" );
    case LocalFolders::Outbox: return QLatin1String( "outbox" );
    case LocalFolders::SentMail: return QLatin1String( "sent-mail" );
    case LocalFolders::Trash: return QLatin1String( "trash" );
    case LocalFolders::Drafts: return QLatin1String( "drafts" );
    case LocalFolders::Templates: return QLatin1String( "templates" );
    default: Q_ASSERT( false ); return QString();
  }
}

QString Akonadi::displayNameForType( int type )
{
  switch( type ) {
    case LocalFolders::Root: return i18nc( "local mail folder", "Local Folders" );
    case LocalFolders::Inbox: return i18nc( "local mail folder", "inbox" );
    case LocalFolders::Outbox: return i18nc( "local mail folder", "outbox" );
    case LocalFolders::SentMail: return i18nc( "local mail folder", "sent-mail" );
    case LocalFolders::Trash: return i18nc( "local mail folder", "trash" );
    case LocalFolders::Drafts: return i18nc( "local mail folder", "drafts" );
    case LocalFolders::Templates: return i18nc( "local mail folder", "templates" );
    default: Q_ASSERT( false ); return QString();
  }
}

QString Akonadi::iconNameForType( int type )
{
  // Icons imitating KMail.
  switch( type ) {
    case LocalFolders::Root: return QLatin1String( "folder" );
    case LocalFolders::Inbox: return QLatin1String( "mail-folder-inbox" );
    case LocalFolders::Outbox: return QLatin1String( "mail-folder-outbox" );
    case LocalFolders::SentMail: return QLatin1String( "mail-folder-sent" );
    case LocalFolders::Trash: return QLatin1String( "user-trash" );
    case LocalFolders::Drafts: return QLatin1String( "document-properties" );
    case LocalFolders::Templates: return QLatin1String( "document-new" );
    default: Q_ASSERT( false ); return QString();
  }
}

void Akonadi::setCollectionAttributes( Collection &col, int type )
{
  Q_ASSERT( type >= 0 && type < LocalFolders::LastDefaultType );

  {
    EntityDisplayAttribute *attr = new EntityDisplayAttribute;
    attr->setIconName( iconNameForType( type ) );
    attr->setDisplayName( displayNameForType( type ) );
    col.addAttribute( attr );
  }

  {
    LocalFolderAttribute *attr = new LocalFolderAttribute;
    attr->setFolderType( type );
    col.addAttribute( attr );
  }
}

bool Akonadi::releaseLock()
{
  return QDBusConnection::sessionBus().unregisterService( DBUS_SERVICE_NAME );
}

#include "localfoldershelperjobs_p.moc"
