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

#include "localfoldersbuildjob_p.h"
#include "localfolderssettings.h"

#include <QDBusConnection>
#include <QObject>
#include <QSet>
#include <QTimer>

#include <KDebug>
#include <KGlobal>
#include <KLocalizedString>
#include <KStandardDirs>

#include <akonadi/monitor.h>

#define DBUS_SERVICE_NAME QLatin1String( "org.kde.pim.LocalFolders" )

using namespace Akonadi;

typedef LocalFoldersSettings Settings;

/**
  @internal
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
    Collection::List defaultFolders;
    QSet<KJob*> pendingJobs;
    Monitor *monitor;

    /**
      If this is the main instance, attempts to build and fetch the local
      folder structure.  Otherwise, it waits for the structure to be created
      by the main instance, and then just fetches it.

      Will emit foldersReady() when the folders are ready.
    */
    void prepare(); // slot

    // slots:
    void buildResult( KJob *job );
    void collectionRemoved( const Collection &col );

    /**
      Schedules a prepare() in 1 second.
      Called when this is not the main instance and we need to wait, or when
      something disappeared and needs to be recreated.
    */
    void schedulePrepare();

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

Collection LocalFolders::root() const
{
  return folder( Root );
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

Collection LocalFolders::folder( int type ) const
{
  Q_ASSERT( d->ready );
  Q_ASSERT( type >= 0 && type < LastDefaultType );
  return d->defaultFolders[ type ];
}



LocalFoldersPrivate::LocalFoldersPrivate()
  : instance( new LocalFolders(this) )
{
  isMainInstance = false;
  ready = false;
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
    kDebug() << "Already ready.  Emitting foldersReady().";
    emit instance->foldersReady();
    return;
  }
  if( preparing ) {
    kDebug() << "Already preparing.";
    return;
  }

  // Try to grab main instance status (if previous main instance quit).
  if( !isMainInstance ) {
    isMainInstance = QDBusConnection::sessionBus().registerService( DBUS_SERVICE_NAME );
  }

  // Fetch / build the folder structure.
  {
    kDebug() << "Preparing. isMainInstance" << isMainInstance;
    preparing = true;
    scheduled = false;
    LocalFoldersBuildJob *bjob = new LocalFoldersBuildJob( isMainInstance, instance );
    QObject::connect( bjob, SIGNAL(result(KJob*)), instance, SLOT(buildResult(KJob*)) );
    // auto-starts
  }
}

void LocalFoldersPrivate::buildResult( KJob *job )
{
  if( job->error() ) {
    kDebug() << "BuildJob failed with error" << job->errorString();
    schedulePrepare();
    return;
  }

  // Get the folders from the job.
  {
    Q_ASSERT( dynamic_cast<LocalFoldersBuildJob*>( job ) );
    LocalFoldersBuildJob *bjob = static_cast<LocalFoldersBuildJob*>( job );
    Q_ASSERT( defaultFolders.isEmpty() );
    defaultFolders = bjob->defaultFolders();
  }

  // Verify everything.
  {
    Q_ASSERT( defaultFolders.count() == LocalFolders::LastDefaultType );
    Collection::Id rootId = defaultFolders[ LocalFolders::Root ].id();
    Q_ASSERT( rootId >= 0 );
    for( int type = 1; type < LocalFolders::LastDefaultType; type++ ) {
      Q_ASSERT( defaultFolders[ type ].isValid() );
      Q_ASSERT( defaultFolders[ type ].parent() == rootId );
    }
  }

  // Connect monitor.
  {
    Q_ASSERT( monitor == 0 );
    monitor = new Monitor( instance );
    monitor->setResourceMonitored( Settings::resourceId().toAscii() );
    QObject::connect( monitor, SIGNAL(collectionRemoved(Akonadi::Collection)),
        instance, SLOT(collectionRemoved(Akonadi::Collection)) );
  }

  // Emit ready.
  {
    kDebug() << "Local folders ready.";
    Q_ASSERT( !ready );
    ready = true;
    preparing = false;
    emit instance->foldersReady();
  }
}

void LocalFoldersPrivate::collectionRemoved( const Akonadi::Collection &col )
{
  kDebug() << "id" << col.id();
  if( defaultFolders.contains( col ) ) {
    // These are undeletable folders.  If one of them got removed, it means
    // the entire resource has been removed.
    schedulePrepare();
  }
}

void LocalFoldersPrivate::schedulePrepare()
{
  if( scheduled ) {
    kDebug() << "Prepare already scheduled.";
    return;
  }

  // Clean up.
  {
    if( monitor ) {
      monitor->disconnect( instance );
      monitor->deleteLater();
      monitor = 0;
    }
    defaultFolders.clear();
  }

  // Schedule prepare in 1s.
  {
    kDebug() << "Scheduling prepare.";
    ready = false;
    preparing = false;
    scheduled = true;
    QTimer::singleShot( 1000, instance, SLOT( prepare() ) );
  }
}

#include "localfolders.moc"
