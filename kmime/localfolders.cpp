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

#include "localfolders.h"

#include "localfolderssettings.h"

#include <QApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QObject>
#include <QTimer>
#include <QVariant>

#include <KDebug>
#include <KGlobal>
#include <KLocalizedString>
#include <KStandardDirs>

#include <akonadi/agentinstance.h>
#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/agentmanager.h>
#include <akonadi/collectioncreatejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/monitor.h>

#include <resourcesynchronizationjob.h> // copied from playground/pim/akonaditest

#define DBUS_SERVICE_NAME QLatin1String( "org.kde.pim.LocalFolders" )


using namespace Akonadi;

typedef LocalFoldersSettings Settings;


/**
 * Private class that helps to provide binary compatibility between releases.
 * @internal
 */
class Akonadi::LocalFoldersPrivate
{
  public:
    LocalFoldersPrivate();
    ~LocalFoldersPrivate();

    LocalFolders *instance;
    bool ready;
    bool preparing;
    bool scheduled;
    bool isMainInstance;
    Collection rootMaildir;
    QMultiHash<int, Collection> folders;
    QSet<KJob*> pendingJobs;
    Monitor *monitor;

    /**
      If this is the main instance, attempts to create the resource and collections
      if necessary, then fetches them.
      If this is not the main instance, waits for them to be created by the main
      instance, and then fetches them.

      Will emit foldersReady() when done
    */
    void prepare();

    /**
      Schedules a prepare() in 1 second.
      Called when this is not the main instance and we need to wait, or when
      something disappeared and needs to be recreated.
    */
    void schedulePrepare(); // slot

    /**
      Creates the maildir resource, if it is not found.
    */
    void createResourceIfNeeded();

    /**
      Creates the outbox and sent-mail collections, if they are not present.
    */
    void createCollectionsIfNeeded();

    /**
      Creates a Monitor to watch the resource and connects to its signals.
      This is used to watch for evil users deleting the resource / outbox / etc.
    */
    void connectMonitor();

    /**
      Fetches the collections of the maildir resource.
      There is one root collection, which contains the outbox and sent-mail
      collections.
     */
    void fetchCollections();

    void resourceCreateResult( KJob *job );
    void resourceSyncResult( KJob *job );
    void collectionCreateResult( KJob *job );
    void collectionFetchResult( KJob *job );

    /**
      Returns the English name for a default (i.e. non-custom) folder type.
    */
    static QString nameForType( int type );

    /**
      Returns the Type for an English name for a folder type.
    */
    static LocalFolders::Type typeForName( const QString &name );
};


K_GLOBAL_STATIC( LocalFoldersPrivate, sInstance )


LocalFolders::LocalFolders( LocalFoldersPrivate *dd )
  : QObject()
  , d( dd )
{
}

LocalFolders *LocalFolders::self()
{
  return sInstance->instance;
}

void LocalFolders::fetch()
{
  d->prepare();
}

bool LocalFolders::isReady() const
{
  return d->ready;
}

Collection LocalFolders::inbox() const
{
  return folder( Inbox );
}

Collection LocalFolders::outbox() const
{
  return folder( Outbox );
}

Collection LocalFolders::sentMail() const
{
  return folder( SentMail );
}

Collection LocalFolders::trash() const
{
  return folder( Trash );
}

Collection LocalFolders::drafts() const
{
  return folder( Drafts );
}

Collection LocalFolders::templates() const
{
  return folder( Templates );
}

#if 0
Collection LocalFolders::folder( const QString &name ) const
{
  Q_ASSERT( d->ready );
  Q_FOREACH( const Collection &col, d->folders.values() ) {
    if( col.name() == name ) {
      return col;
    }
  }
}
#endif

Collection LocalFolders::folder( Type type ) const
{
  Q_ASSERT( type < Custom );
  Q_ASSERT( d->ready );
  if( d->folders.contains( type ) ) {
    return d->folders.value( type );
  } else {
    kWarning() << "Non-existent folder of type" << type;
    return Collection();
  }
}

Collection::List LocalFolders::folders( Type type ) const
{
  Q_ASSERT( d->ready );
  if( d->folders.contains( type ) ) {
    return d->folders.values( type );
  } else {
    kWarning() << "No folders of type" << type;
    return Collection::List();
  }
}



LocalFoldersPrivate::LocalFoldersPrivate()
  : instance( new LocalFolders(this) )
{
  isMainInstance = QDBusConnection::sessionBus().registerService( DBUS_SERVICE_NAME );

  ready = false;
  // prepare() expects these
  preparing = false;
  monitor = 0;
  prepare();
}

LocalFoldersPrivate::~LocalFoldersPrivate()
{
  delete instance;
}

void LocalFoldersPrivate::prepare()
{
  if( ready ) {
    //kDebug() << "Already ready.  Emitting foldersReady().";
    emit instance->foldersReady();
    return;
  }
  if( preparing ) {
    kDebug() << "Already preparing.";
    return;
  }
  kDebug() << "Preparing. isMainInstance" << isMainInstance;
  preparing = true;
  scheduled = false;

  rootMaildir = Collection( -1 );
  createResourceIfNeeded();
}

void LocalFoldersPrivate::schedulePrepare()
{
  if( scheduled ) {
    kDebug() << "Prepare already scheduled.";
    return;
  }

  kDebug() << "Scheduling prepare.";

  if( monitor ) {
    monitor->disconnect( instance );
    monitor->deleteLater();
    monitor = 0;
  }

  ready = false;
  preparing = false;
  scheduled = true;
  QTimer::singleShot( 1000, instance, SLOT( prepare() ) );
}

void LocalFoldersPrivate::createResourceIfNeeded()
{
  Q_ASSERT( preparing );

  // Another instance might have created the resource and updated the config.
  Settings::self()->readConfig();
  kDebug() << "Resource from config:" << Settings::resourceId();

  // check that the maildir resource exists
  AgentInstance resource = AgentManager::self()->instance( Settings::resourceId() );
  if( !resource.isValid() ) {
    // Try to grab main instance status (if previous main instance quit).
    if( !isMainInstance ) {
      isMainInstance = QDBusConnection::sessionBus().registerService( DBUS_SERVICE_NAME );
      if( isMainInstance ) {
        kDebug() << "I have become the main instance.";
      }
    }

    // Create resource if main instance.
    if( !isMainInstance ) {
      kDebug() << "Waiting for the main instance to create the resource.";
      schedulePrepare();
    } else {
      kDebug() << "Creating maildir resource.";
      AgentType type = AgentManager::self()->type( QString::fromLatin1( "akonadi_maildir_resource" ) );
      AgentInstanceCreateJob *job = new AgentInstanceCreateJob( type );
      QObject::connect( job, SIGNAL( result( KJob * ) ),
        instance, SLOT( resourceCreateResult( KJob * ) ) );
      // this is not an Akonadi::Job, so we must start it ourselves
      job->start();
    }
  } else {
    connectMonitor();
  }
}

void LocalFoldersPrivate::createCollectionsIfNeeded()
{
  Q_ASSERT( preparing ); // but I may not be the main instance
  Q_ASSERT( rootMaildir.isValid() );

  for( int type = 0; type < LocalFolders::LastDefaultType; type++ ) {
    if( !folders.contains( type ) ) {
      kDebug() << "Creating" << nameForType( type ) << "collection.";
      Collection col;
      col.setParent( rootMaildir );
      col.setName( nameForType( type ) );
      col.setContentMimeTypes( QStringList( QLatin1String( "message/rfc822" ) ) );
      CollectionCreateJob *cjob = new CollectionCreateJob( col );
      QObject::connect( cjob, SIGNAL(result(KJob*)),
        instance, SLOT(collectionCreateResult(KJob*)) );
      pendingJobs.insert( cjob );
    }
  }

  if( pendingJobs.isEmpty() ) {
    // Everything is ready (created and fetched).
    kDebug() << "Local folders ready. resourceId" << Settings::resourceId();
    for( int type = 0; type < LocalFolders::LastDefaultType; type++ ) {
      kDebug() << nameForType( type ) << "collection has id" << folders.value(type).id();
    }

    Q_ASSERT( !ready );
    ready = true;
    preparing = false;
    Settings::self()->writeConfig();
    emit instance->foldersReady();
  }
}

void LocalFoldersPrivate::connectMonitor()
{
  Q_ASSERT( preparing ); // but I may not be the main instance
  Q_ASSERT( monitor == 0 );
  monitor = new Monitor( instance );
  monitor->setResourceMonitored( Settings::resourceId().toAscii() );
  QObject::connect( monitor, SIGNAL( collectionRemoved( Akonadi::Collection ) ),
      instance, SLOT( schedulePrepare() ) );
  kDebug() << "Connected monitor.";
  fetchCollections();
}

void LocalFoldersPrivate::fetchCollections()
{
  Q_ASSERT( preparing ); // but I may not be the main instance
  kDebug() << "Fetching collections in maildir resource.";

  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
  job->setResource( Settings::resourceId() ); // limit search
  QObject::connect( job, SIGNAL( result( KJob * ) ),
      instance, SLOT( collectionFetchResult( KJob * ) ) );
}

void LocalFoldersPrivate::resourceCreateResult( KJob *job )
{
  Q_ASSERT( isMainInstance );
  Q_ASSERT( preparing );
  if( job->error() ) {
    kFatal() << "AgentInstanceCreateJob failed to make a maildir resource for us.";
  }

  AgentInstanceCreateJob *createJob = static_cast<AgentInstanceCreateJob *>( job );
  AgentInstance agent = createJob->instance();
  Settings::setResourceId( agent.identifier() );
  kDebug() << "Created maildir resource with id" << Settings::resourceId();

  // configure the resource
  agent.setName( i18n( "Local Mail Folders" ) );
  QDBusInterface conf( QString::fromLatin1( "org.freedesktop.Akonadi.Resource." ) + Settings::resourceId(),
                       QString::fromLatin1( "/Settings" ),
                       QString::fromLatin1( "org.kde.Akonadi.Maildir.Settings" ) );
  QDBusReply<void> reply = conf.call( QString::fromLatin1( "setPath" ),
                                      KGlobal::dirs()->localxdgdatadir() + QString::fromLatin1( "mail" ) );
  if( !reply.isValid() ) {
    kFatal() << "Failed to set the root maildir.";
  }
  agent.reconfigure();

  // sync the resource
  ResourceSynchronizationJob *sjob = new ResourceSynchronizationJob( agent );
  QObject::connect( sjob, SIGNAL( result( KJob* ) ),
      instance, SLOT( resourceSyncResult( KJob* ) ) );
  sjob->start(); // non-Akonadi
}

void LocalFoldersPrivate::resourceSyncResult( KJob *job )
{
  Q_ASSERT( isMainInstance );
  Q_ASSERT( preparing );
  if( job->error() ) {
    kFatal() << "ResourceSynchronizationJob failed.";
  }

  connectMonitor();
}

void LocalFoldersPrivate::collectionCreateResult( KJob *job )
{
  Q_ASSERT( isMainInstance );
  if( job->error() ) {
    kFatal() << "CollectionCreateJob failed to make a collection for us.";
  }

  Q_ASSERT( pendingJobs.contains( job ) );
  pendingJobs.remove( job );
  if( pendingJobs.isEmpty() ) {
    // Done creating.  Refetch everything.
    fetchCollections();
  }
}

void LocalFoldersPrivate::collectionFetchResult( KJob *job )
{
  Q_ASSERT( preparing ); // but I may not be the main instance
  CollectionFetchJob *fetchJob = static_cast<CollectionFetchJob *>( job );
  Collection::List cols = fetchJob->collections();

  kDebug() << "CollectionFetchJob fetched" << cols.count() << "collections.";

  folders.clear();
  Q_FOREACH( const Collection &col, cols ) {
    if( col.parent() == Collection::root().id() ) {
      rootMaildir = col;
      kDebug() << "Fetched root maildir collection.";
    } else {
      // Try to guess folder type.
      LocalFolders::Type type = typeForName( col.name() );
      kDebug() << "Fetched" << nameForType( type ) << "collection.";
      if( type != LocalFolders::Custom && folders.contains( type ) ) {
        kDebug() << "But I have this type already, so making it Custom.";
        type = LocalFolders::Custom;
      }
      folders.insert( type, col );
    }
  }

  if( !rootMaildir.isValid() ) {
    kFatal() << "Failed to fetch root maildir collection.";
  }

  createCollectionsIfNeeded();
}


// static
QString LocalFoldersPrivate::nameForType( int type )
{
  Q_ASSERT( type >= 0 && type < LocalFolders::LastDefaultType );
  switch( type ) {
    case LocalFolders::Inbox: return QLatin1String( "inbox" );
    case LocalFolders::Outbox: return QLatin1String( "outbox" );
    case LocalFolders::SentMail: return QLatin1String( "sent-mail" );
    case LocalFolders::Trash: return QLatin1String( "trash" );
    case LocalFolders::Drafts: return QLatin1String( "drafts" );
    case LocalFolders::Templates: return QLatin1String( "templates" );
    default: Q_ASSERT( false ); return QString();
  }
}

//static
LocalFolders::Type LocalFoldersPrivate::typeForName( const QString &name )
{
  if( name == QLatin1String( "inbox" ) ) {
    return LocalFolders::Inbox;
  } else if( name == QLatin1String( "outbox" ) ) {
    return LocalFolders::Outbox;
  } else if( name == QLatin1String( "sent-mail" ) ) {
    return LocalFolders::SentMail;
  } else if( name == QLatin1String( "trash" ) ) {
    return LocalFolders::Trash;
  } else if( name == QLatin1String( "drafts" ) ) {
    return LocalFolders::Drafts;
  } else if( name == QLatin1String( "templates" ) ) {
    return LocalFolders::Templates;
  } else {
    return LocalFolders::Custom;
  }
}


#include "localfolders.moc"
