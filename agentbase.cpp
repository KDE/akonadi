/*
    Copyright (c) 2006 Till Adam <adam@kde.org>
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2007 Bruno Virlet <bruno.virlet@gmail.com>
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "agentbase.h"
#include "agentbase_p.h"

#include "agent.h"
#include "agentadaptor.h"
#include "monitor_p.h"
#include "xdgbasedirs_p.h"

#include "session.h"
#include "changerecorder.h"
#include "itemfetchjob.h"

#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <klocale.h>

#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtDBus/QtDBus>

#include <signal.h>
#include <stdlib.h>

using namespace Akonadi;

static AgentBase *sAgentBase = 0;

AgentBase::Observer::Observer()
{
}

AgentBase::Observer::~Observer()
{
}

void AgentBase::Observer::itemAdded( const Item &item, const Collection &collection )
{
  kDebug() << "sAgentBase=" << (void*) sAgentBase << "this=" << (void*) this;
  Q_UNUSED( item );
  Q_UNUSED( collection );
  if ( sAgentBase != 0 )
    sAgentBase->changeProcessed();
}

void AgentBase::Observer::itemChanged( const Item &item, const QStringList &partIdentifiers )
{
  kDebug() << "sAgentBase=" << (void*) sAgentBase << "this=" << (void*) this;
  Q_UNUSED( item );
  Q_UNUSED( partIdentifiers );
  if ( sAgentBase != 0 )
    sAgentBase->changeProcessed();
}

void AgentBase::Observer::itemRemoved( const Item &item )
{
  kDebug() << "sAgentBase=" << (void*) sAgentBase << "this=" << (void*) this;
  Q_UNUSED( item );
  if ( sAgentBase != 0 )
    sAgentBase->changeProcessed();
}

void AgentBase::Observer::collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent )
{
  kDebug() << "sAgentBase=" << (void*) sAgentBase << "this=" << (void*) this;
  Q_UNUSED( collection );
  Q_UNUSED( parent );
  if ( sAgentBase != 0 )
    sAgentBase->changeProcessed();
}

void AgentBase::Observer::collectionChanged( const Collection &collection )
{
  kDebug() << "sAgentBase=" << (void*) sAgentBase << "this=" << (void*) this;
  Q_UNUSED( collection );
  if ( sAgentBase != 0 )
    sAgentBase->changeProcessed();
}

void AgentBase::Observer::collectionRemoved( const Collection &collection )
{
  kDebug() << "sAgentBase=" << (void*) sAgentBase << "this=" << (void*) this;
  Q_UNUSED( collection );
  if ( sAgentBase != 0 )
    sAgentBase->changeProcessed();
}

//@cond PRIVATE

AgentBasePrivate::AgentBasePrivate( AgentBase *parent )
  : q_ptr( parent ),
    mSettings( 0 ),
    mObserver( 0 )
{
}

AgentBasePrivate::~AgentBasePrivate()
{
  monitor->setConfig( 0 );
  delete mSettings;
}

void AgentBasePrivate::init()
{
  Q_Q( AgentBase );
  mTracer = new org::kde::Akonadi::Tracer( QLatin1String( "org.kde.Akonadi" ), QLatin1String( "/tracing" ),
                                           QDBusConnection::sessionBus(), q );

  new AgentAdaptor( q );
  if ( !QDBusConnection::sessionBus().registerObject( QLatin1String( "/" ), q, QDBusConnection::ExportAdaptors ) )
    q->error( QString::fromLatin1( "Unable to register object at dbus: %1" ).arg( QDBusConnection::sessionBus().lastError().message() ) );

  mSettings = new QSettings( QString::fromLatin1( "%1/agent_config_%2" ).arg( XdgBaseDirs::saveDir( "config", QLatin1String( "akonadi" ) ), mId ), QSettings::IniFormat );

  session = new Session( mId.toLatin1(), q );
  monitor = new ChangeRecorder( q );
  monitor->ignoreSession( session );
  monitor->setConfig( mSettings );

  connect( monitor, SIGNAL( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ),
           SLOT( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ) );
  connect( monitor, SIGNAL( itemChanged( const Akonadi::Item&, const QStringList& ) ),
           SLOT( itemChanged( const Akonadi::Item&, const QStringList& ) ) );
  connect( monitor, SIGNAL( itemRemoved( const Akonadi::Item& ) ),
           SLOT( itemRemoved( const Akonadi::Item& ) ) );
  connect( monitor, SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)),
           SLOT(collectionAdded(Akonadi::Collection,Akonadi::Collection)) );
  connect( monitor, SIGNAL( collectionChanged( const Akonadi::Collection& ) ),
           SLOT( collectionChanged( const Akonadi::Collection& ) ) );
  connect( monitor, SIGNAL( collectionRemoved( const Akonadi::Collection& ) ),
           SLOT( collectionRemoved( const Akonadi::Collection& ) ) );

  QTimer::singleShot( 0, q, SLOT(delayedInit()) );
}

void AgentBasePrivate::delayedInit()
{
  if ( !QDBusConnection::sessionBus().registerService( QLatin1String( "org.kde.Akonadi.Agent." ) + mId ) )
    kFatal() << "Unable to register service at dbus:" << QDBusConnection::sessionBus().lastError().message();
}

void AgentBasePrivate::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  kDebug() << "mObserver=" << (void*) mObserver << "this=" << (void*) this;
  if ( mObserver != 0 )
    mObserver->itemAdded( item, collection );
}

void AgentBasePrivate::itemChanged( const Akonadi::Item &item, const QStringList &partIdentifiers )
{
  kDebug() << "mObserver=" << (void*) mObserver << "this=" << (void*) this;
  if ( mObserver != 0 )
    mObserver->itemChanged( item, partIdentifiers );
}

void AgentBasePrivate::itemRemoved( const Akonadi::Item &item )
{
  kDebug() << "mObserver=" << (void*) mObserver << "this=" << (void*) this;
  if ( mObserver != 0 )
    mObserver->itemRemoved( item );
}

void AgentBasePrivate::collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent )
{
  kDebug() << "mObserver=" << (void*) mObserver << "this=" << (void*) this;
  if ( mObserver != 0 )
    mObserver->collectionAdded( collection, parent );
}

void AgentBasePrivate::collectionChanged( const Akonadi::Collection &collection )
{
  kDebug() << "mObserver=" << (void*) mObserver << "this=" << (void*) this;
  if ( mObserver != 0 )
    mObserver->collectionChanged( collection );
}

void AgentBasePrivate::collectionRemoved( const Akonadi::Collection &collection )
{
  kDebug() << "mObserver=" << (void*) mObserver << "this=" << (void*) this;
  if ( mObserver != 0 )
    mObserver->collectionRemoved( collection );
}

AgentBase::AgentBase( const QString & id )
  : d_ptr( new AgentBasePrivate( this ) )
{
  sAgentBase = this;
  d_ptr->mId = id;
  d_ptr->init();
}

// @endcond

AgentBase::AgentBase( AgentBasePrivate* d, const QString &id ) :
    d_ptr( d )
{
  sAgentBase = this;
  d_ptr->mId = id;
  d_ptr->init();
}

AgentBase::~AgentBase()
{
  delete d_ptr;
}

static char* sAppName = 0;

QString AgentBase::parseArguments( int argc, char **argv )
{
  QString identifier;
  if ( argc < 3 ) {
    kDebug( 5250 ) << "Not enough arguments passed...";
    exit( 1 );
  }

  for ( int i = 1; i < argc - 1; ++i ) {
    if ( QLatin1String( argv[ i ] ) == QLatin1String( "--identifier" ) )
      identifier = QLatin1String( argv[ i + 1 ] );
  }

  if ( identifier.isEmpty() ) {
    kDebug( 5250 ) << "Identifier argument missing";
    exit( 1 );
  }

  sAppName = qstrdup( identifier.toLatin1().constData() );
  KCmdLineArgs::init( argc, argv, sAppName, 0, ki18n("Akonadi Agent"),"0.1" ,
                      ki18n("Akonadi Agent") );

  KCmdLineOptions options;
  options.add("identifier <argument>", ki18n("Agent identifier"));
  KCmdLineArgs::addCmdLineOptions( options );

  return identifier;
}

int AgentBase::init( AgentBase *r )
{
  QApplication::setQuitOnLastWindowClosed( false );
  int rv = kapp->exec();
  delete r;
  delete[] sAppName;
  return rv;
}

void AgentBase::configure( WId windowId )
{
  Q_UNUSED( windowId );
}

WId AgentBase::winIdForDialogs() const
{
  bool registered = QDBusConnection::sessionBus().interface()->isServiceRegistered( QLatin1String("org.kde.akonaditray") );
  if ( !registered )
    return 0;

  QDBusInterface dbus( QLatin1String("org.kde.akonaditray"), QLatin1String("/Actions"),
                       QLatin1String("org.kde.Akonadi.Tray") );
  QDBusMessage reply = dbus.call( QLatin1String("getWinId") );

  if ( reply.type() == QDBusMessage::ErrorMessage )
    return 0;

  WId winid = (WId)reply.arguments().at( 0 ).toLongLong();
  return winid;
}

void AgentBase::quit()
{
  Q_D( AgentBase );
  aboutToQuit();

  if ( d->mSettings ) {
    d->monitor->setConfig( 0 );
    d->mSettings->sync();
  }

  QCoreApplication::exit( 0 );
}

void AgentBase::aboutToQuit()
{
}

void AgentBase::cleanup()
{
  Q_D( AgentBase );
  const QString fileName = d->mSettings->fileName();

  /**
   * First destroy the settings object...
   */
  d->monitor->setConfig( 0 );
  delete d->mSettings;
  d->mSettings = 0;

  /**
   * ... then remove the file from hd.
   */
  QFile::remove( fileName );

  QCoreApplication::quit();
}

void AgentBase::registerObserver( Observer *observer )
{
  kDebug() << "observer=" << (void*) observer << "this=" << (void*) this;
  d_ptr->mObserver = observer;
}

QString AgentBase::identifier() const
{
  return d_ptr->mId;
}

Session* AgentBase::session() const
{
  return d_ptr->session;
}

void AgentBase::warning( const QString& message )
{
  Q_D( AgentBase );
  d->mTracer->warning( QString::fromLatin1( "AgentBase(%1)" ).arg( d->mId ), message );
}

void AgentBase::error( const QString& message )
{
  Q_D( AgentBase );
  d->mTracer->error( QString::fromLatin1( "AgentBase(%1)" ).arg( d->mId ), message );
}

ChangeRecorder * AgentBase::changeRecorder() const
{
  return d_ptr->monitor;
}

void AgentBase::changeProcessed()
{
  d_ptr->monitor->changeProcessed();
  QTimer::singleShot( 0, d_ptr->monitor, SLOT(replayNext()) );
}

#include "agent.moc"
#include "agentbase.moc"
#include "agentbase_p.moc"
