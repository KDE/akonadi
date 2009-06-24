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

#include "localfoldersbuildjob_p.h"

#include "localfolders.h"
#include "localfolderssettings.h"

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QVariant>

#include <KDebug>
#include <KLocalizedString>
#include <KStandardDirs>

#include <akonadi/agentinstance.h>
#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/agentmanager.h>
#include <akonadi/collectioncreatejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/entitydisplayattribute.h>

#include <resourcesynchronizationjob.h> // copied from playground/pim/akonaditest

using namespace Akonadi;

typedef LocalFoldersSettings Settings;

/**
  @internal
*/
class Akonadi::LocalFoldersBuildJob::Private
{
  public:
    Private( LocalFoldersBuildJob *qq )
      : q( qq )
    {
      for( int type = 0; type < LocalFolders::LastDefaultType; type++ ) {
        defaultFolders.append( Collection() );
      }
    }

    void resourceCreateResult( KJob *job ); // slot
    void resourceSyncResult( KJob *job ); // slot
    void fetchCollections();
    void collectionFetchResult( KJob *job ); // slot
    void createAndConfigureCollections();
    void collectionCreateResult( KJob *job ); // slot
    void collectionModifyResult( KJob *job ); // slot

    // May be called for any default type, including Root.
    static QString nameForType( int type );

    // May be called for any default type, including Root.
    static QString iconNameForType( int type );

    // May be called for any default or custom type, other than Root.
    static LocalFolders::Type typeForName( const QString &name );

    LocalFoldersBuildJob *q;
    bool canBuild;
    Collection::List defaultFolders;
    //QSet<Collection> customFolders;
    QSet<KJob*> pendingJobs;

};



LocalFoldersBuildJob::LocalFoldersBuildJob( bool canBuild, QObject *parent )
  : TransactionSequence( parent )
  , d( new Private( this ) )
{
  d->canBuild = canBuild;
}

LocalFoldersBuildJob::~LocalFoldersBuildJob()
{
  delete d;
}

const Collection::List &LocalFoldersBuildJob::defaultFolders() const
{
  return d->defaultFolders;
}

#if 0
const QSet<Collection> &LocalFoldersBuildJob::customFolders() const
{
  return d->customFolders;
}
#endif

void LocalFoldersBuildJob::doStart()
{
  // Update config and check if resource exists.
  Settings::self()->readConfig();
  kDebug() << "resourceId from settings" << Settings::resourceId();
  AgentInstance resource = AgentManager::self()->instance( Settings::resourceId() );
  if( resource.isValid() ) {
    // Good, the resource exists.
    d->fetchCollections();
  } else {
    // Create resource if allowed.
    if( !d->canBuild ) {
      setError( UserDefinedError );
      setErrorText( QLatin1String( "Resource not found, and creating not allowed." ) );
      emitResult();
    } else {
      kDebug() << "Creating maildir resource.";
      AgentType type = AgentManager::self()->type( QString::fromLatin1( "akonadi_maildir_resource" ) );
      AgentInstanceCreateJob *job = new AgentInstanceCreateJob( type, this );
      connect( job, SIGNAL(result(KJob*)), this, SLOT(resourceCreateResult(KJob*)) );
      job->start(); // non-Akonadi::Job
    }
  }
}



void LocalFoldersBuildJob::Private::resourceCreateResult( KJob *job )
{
  Q_ASSERT( canBuild );
  if( job->error() ) {
    q->setError( UserDefinedError );
    q->setErrorText( QLatin1String( "Resource creation failed." ) );
    q->emitResult();
  } else {
    AgentInstance agent;
    
    // Get the resource instance, and save its ID to config.
    {
      Q_ASSERT( dynamic_cast<AgentInstanceCreateJob*>( job ) );
      AgentInstanceCreateJob *cjob = static_cast<AgentInstanceCreateJob*>( job );
      agent = cjob->instance();
      Settings::setResourceId( agent.identifier() );
      kDebug() << "Created maildir resource with id" << Settings::resourceId();
    }

    // Configure the resource.
    {
      agent.setName( i18n( "Local Mail Folders" ) );
      QDBusInterface conf( QString::fromLatin1( "org.freedesktop.Akonadi.Resource." ) + Settings::resourceId(),
                           QString::fromLatin1( "/Settings" ),
                           QString::fromLatin1( "org.kde.Akonadi.Maildir.Settings" ) );
      QDBusReply<void> reply = conf.call( QString::fromLatin1( "setPath" ),
                                          KGlobal::dirs()->localxdgdatadir() + QString::fromLatin1( "mail" ) );
      if( !reply.isValid() ) {
        q->setError( UserDefinedError );
        q->setErrorText( QLatin1String( "Failed to set the root maildir via D-Bus." ) );
        q->emitResult();
      } else {
        agent.reconfigure();
      }
    }

    // Sync the resource.
    if( !q->error() )
    {
      ResourceSynchronizationJob *sjob = new ResourceSynchronizationJob( agent, q );
      QObject::connect( sjob, SIGNAL(result(KJob*)), q, SLOT(resourceSyncResult(KJob*)) );
      sjob->start(); // non-Akonadi
    }
  }
}

void LocalFoldersBuildJob::Private::resourceSyncResult( KJob *job )
{
  Q_ASSERT( canBuild );
  if( job->error() ) {
    q->setError( UserDefinedError );
    q->setErrorText( QLatin1String( "ResourceSynchronizationJob failed." ) );
    q->emitResult();
  } else {
    fetchCollections();
  }
}

void LocalFoldersBuildJob::Private::fetchCollections()
{
  kDebug() << "Fetching collections in the maildir resource.";

  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive, q );
  job->setResource( Settings::resourceId() ); // limit search
  QObject::connect( job, SIGNAL(result(KJob*)), q, SLOT(collectionFetchResult(KJob*)) );
}

void LocalFoldersBuildJob::Private::collectionFetchResult( KJob *job )
{
  QSet<Collection> cols;

  // Get the root maildir collection, and put all the other ones in cols.
  {
    Q_ASSERT( dynamic_cast<CollectionFetchJob*>( job ) );
    CollectionFetchJob *fjob = static_cast<CollectionFetchJob *>( job );
    Q_FOREACH( const Collection &col, fjob->collections() ) {
      if( col.parent() == Collection::root().id() ) {
        // This is the root maildir collection.
        Q_ASSERT( !defaultFolders[ LocalFolders::Root ].isValid() );
        defaultFolders[ LocalFolders::Root ] = col;
      } else {
        // This is some local folder.
        cols.insert( col );
      }
    }
  }

  // Go no further if the root maildir was not found.
  if( !defaultFolders[ LocalFolders::Root ].isValid() ) {
    q->setError( UserDefinedError );
    q->setErrorText( QLatin1String( "Failed to fetch root maildir collection." ) );
    q->emitResult();
    return;
  }

  // Get the default folders.  They must be direct children of the root maildir,
  // and have appropriate names.  Leave all the other folders in cols.
  Q_FOREACH( const Collection &col, cols ) {
    if( col.parent() == defaultFolders[ LocalFolders::Root ].id() ) {
      // This is a direct child; guess its type.
      LocalFolders::Type type = typeForName( col.name() );
      if( type != LocalFolders::Custom ) {
        // This is one of the default folders.
        Q_ASSERT( !defaultFolders[ type ].isValid() );
        defaultFolders[ type ] = col;
        cols.remove( col );
      }
    }
  }

  // Everything left in cols is custom folders.
  //customFolders = cols;

  // Some collections may still need to be created (if the evil user deleted
  // them from disk, for example), and some may need to be configured (rights,
  // icon, i18n'ed name).
  if( canBuild ) {
    createAndConfigureCollections();
  } else {
    for( int type = 0; type < LocalFolders::LastDefaultType; type++ ) {
      if( !defaultFolders[ type ].isValid() ) {
        q->setError( UserDefinedError );
        q->setErrorText( QString::fromLatin1( "Failed to fetch %1 collection." ).arg( nameForType( type ) ) );
        q->emitResult();
        break;
      }
    }

    // If all collections are valid, we are done.
    if( !q->error() ) {
      q->commit();
    }
  }
}

void LocalFoldersBuildJob::Private::createAndConfigureCollections()
{
  Q_ASSERT( canBuild );

  // We need the root maildir at least.
  Q_ASSERT( defaultFolders[ LocalFolders::Root ].isValid() );

  for( int type = 0; type < LocalFolders::LastDefaultType; type++ ) {
    if( !defaultFolders[ type ].isValid() ) {
      // This default folder needs to be created.
      kDebug() << "Creating default folder" << nameForType( type );
      Q_ASSERT( type != LocalFolders::Root );
      Collection col;
      col.setParent( defaultFolders[ LocalFolders::Root ].id() );
      col.setName( nameForType( type ) );
      EntityDisplayAttribute *attr = new EntityDisplayAttribute;
      attr->setIconName( iconNameForType( type ) );
      attr->setDisplayName( i18nc( "local mail folder", nameForType( type ).toLatin1() ) );
      col.addAttribute( attr );
      col.setRights( Collection::Rights( Collection::AllRights ^ Collection::CanDeleteCollection ) );
      CollectionCreateJob *cjob = new CollectionCreateJob( col, q );
      QObject::connect( cjob, SIGNAL(result(KJob*)), q, SLOT(collectionCreateResult(KJob*)) );
    } else if( defaultFolders[ type ].rights() & Collection::CanDeleteCollection ) {
      // This default folder needs to be modified.
      kDebug() << "Modifying default folder" << nameForType( type );
      Collection &col = defaultFolders[ type ];
      EntityDisplayAttribute *attr = new EntityDisplayAttribute;
      attr->setIconName( iconNameForType( type ) );
      attr->setDisplayName( i18nc( "local mail folder", nameForType( type ).toLatin1() ) );
      col.addAttribute( attr );
      col.setRights( Collection::Rights( Collection::AllRights ^ Collection::CanDeleteCollection ) );
      CollectionModifyJob *mjob = new CollectionModifyJob( col, q );
      QObject::connect( mjob, SIGNAL(result(KJob*)), q, SLOT(collectionModifyResult(KJob*)) );
    }

    // NOTE: We do not set the mime type here because the maildir resource does that.
    // It sets inode/directory (for child collections) and message/rfc822 (for messages),
    // which is exactly what we want.
  }

  // When those subjobs are finished, we are done.
  Settings::self()->writeConfig();
  q->commit();
}

void LocalFoldersBuildJob::Private::collectionCreateResult( KJob *job )
{
  if( job->error() ) {
    kDebug() << "CollectionCreateJob failed.";
    // TransactionSequence will take care of the error.
    return;
  }

  Q_ASSERT( dynamic_cast<CollectionCreateJob*>( job ) );
  CollectionCreateJob *cjob = static_cast<CollectionCreateJob*>( job );
  const Collection col = cjob->collection();
  int type = typeForName( col.name() );
  Q_ASSERT( type < LocalFolders::LastDefaultType );
  Q_ASSERT( !defaultFolders[ type ].isValid() );
  defaultFolders[ type ] = col;
}

void LocalFoldersBuildJob::Private::collectionModifyResult( KJob *job )
{
  if( job->error() ) {
    kDebug() << "CollectionModifyJob failed.";
    // TransactionSequence will take care of the error.
    return;
  }

  Q_ASSERT( dynamic_cast<CollectionModifyJob*>( job ) );
  // Our collection in defaultFolders[...] is already modified.
  // (And CollectionModifyJob does not provide collection().)
}


// static
QString LocalFoldersBuildJob::Private::nameForType( int type )
{
  switch( type ) {
    case LocalFolders::Root: return QLatin1String( "Local Folders" );
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
QString LocalFoldersBuildJob::Private::iconNameForType( int type )
{
  // Icons imitating KMail.
  switch( type ) {
    case LocalFolders::Root: return QString();
    case LocalFolders::Inbox: return QLatin1String( "mail-folder-inbox" );
    case LocalFolders::Outbox: return QLatin1String( "mail-folder-outbox" );
    case LocalFolders::SentMail: return QLatin1String( "mail-folder-sent" );
    case LocalFolders::Trash: return QLatin1String( "user-trash" );
    case LocalFolders::Drafts: return QLatin1String( "document-properties" );
    case LocalFolders::Templates: return QLatin1String( "document-new" );
    default: Q_ASSERT( false ); return QString();
  }
}

//static
LocalFolders::Type LocalFoldersBuildJob::Private::typeForName( const QString &name )
{
  if( name == QLatin1String( "root" ) ) {
    Q_ASSERT( false );
    return LocalFolders::Root;
  } else if( name == QLatin1String( "inbox" ) ) {
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


#include "localfoldersbuildjob_p.moc"
