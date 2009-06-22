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

#include "controladaptor.h"
#include "statusadaptor.h"
#include "monitor_p.h"
#include "xdgbasedirs_p.h"

#include "session.h"
#include "session_p.h"
#include "changerecorder.h"
#include "itemfetchjob.h"

#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <klocale.h>
#include <kstandarddirs.h>

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
    sAgentBase->d_ptr->changeProcessed();
}

void AgentBase::Observer::itemChanged( const Item &item, const QSet<QByteArray> &partIdentifiers )
{
  kDebug() << "sAgentBase=" << (void*) sAgentBase << "this=" << (void*) this;
  Q_UNUSED( item );
  Q_UNUSED( partIdentifiers );
  if ( sAgentBase != 0 )
    sAgentBase->d_ptr->changeProcessed();
}

void AgentBase::Observer::itemRemoved( const Item &item )
{
  kDebug() << "sAgentBase=" << (void*) sAgentBase << "this=" << (void*) this;
  Q_UNUSED( item );
  if ( sAgentBase != 0 )
    sAgentBase->d_ptr->changeProcessed();
}

void AgentBase::Observer::collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent )
{
  kDebug() << "sAgentBase=" << (void*) sAgentBase << "this=" << (void*) this;
  Q_UNUSED( collection );
  Q_UNUSED( parent );
  if ( sAgentBase != 0 )
    sAgentBase->d_ptr->changeProcessed();
}

void AgentBase::Observer::collectionChanged( const Collection &collection )
{
  kDebug() << "sAgentBase=" << (void*) sAgentBase << "this=" << (void*) this;
  Q_UNUSED( collection );
  if ( sAgentBase != 0 )
    sAgentBase->d_ptr->changeProcessed();
}

void AgentBase::Observer::collectionRemoved( const Collection &collection )
{
  kDebug() << "sAgentBase=" << (void*) sAgentBase << "this=" << (void*) this;
  Q_UNUSED( collection );
  if ( sAgentBase != 0 )
    sAgentBase->d_ptr->changeProcessed();
}

//@cond PRIVATE

AgentBasePrivate::AgentBasePrivate( AgentBase *parent )
  : q_ptr( parent ),
    mStatusCode( AgentBase::Idle ),
    mProgress( 0 ),
    mNeedsNetwork( false ),
    mOnline( false ),
    mSettings( 0 ),
    mObserver( 0 )
{
}

AgentBasePrivate::~AgentBasePrivate()
{
  mMonitor->setConfig( 0 );
  delete mSettings;
}

void AgentBasePrivate::init()
{
  Q_Q( AgentBase );

  /**
   * Create a default session for this process.
   */
  SessionPrivate::createDefaultSession( mId.toLatin1() );

  mTracer = new org::freedesktop::Akonadi::Tracer( QLatin1String( "org.freedesktop.Akonadi" ), QLatin1String( "/tracing" ),
                                           QDBusConnection::sessionBus(), q );

  new ControlAdaptor( q );
  new StatusAdaptor( q );
  if ( !QDBusConnection::sessionBus().registerObject( QLatin1String( "/" ), q, QDBusConnection::ExportAdaptors ) )
    q->error( QString::fromLatin1( "Unable to register object at dbus: %1" ).arg( QDBusConnection::sessionBus().lastError().message() ) );

  mSettings = new QSettings( QString::fromLatin1( "%1/agent_config_%2" ).arg( XdgBaseDirs::saveDir( "config", QLatin1String( "akonadi" ) ), mId ), QSettings::IniFormat );

  mMonitor = new ChangeRecorder( q );
  mMonitor->ignoreSession( Session::defaultSession() );
  mMonitor->itemFetchScope().setCacheOnly( true );
  mMonitor->setConfig( mSettings );

  mOnline = mSettings->value( QLatin1String( "Agent/Online" ), true ).toBool();

  mName = mSettings->value( QLatin1String( "Agent/Name" ) ).toString();
  if (mName.isEmpty()) {
    mName = mSettings->value( QLatin1String( "Resource/Name" ) ).toString();
    if (!mName.isEmpty()) {
      mSettings->remove( QLatin1String( "Resource/Name" ) );
      mSettings->setValue( QLatin1String( "Agent/Name" ), mName );
    }
  }
  

  connect( mMonitor, SIGNAL( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ),
           SLOT( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ) );
  connect( mMonitor, SIGNAL( itemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) ),
           SLOT( itemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) ) );
  connect( mMonitor, SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)),
           SLOT(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)) );
  connect( mMonitor, SIGNAL( itemRemoved( const Akonadi::Item& ) ),
           SLOT( itemRemoved( const Akonadi::Item& ) ) );
  connect( mMonitor, SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)),
           SLOT(collectionAdded(Akonadi::Collection,Akonadi::Collection)) );
  connect( mMonitor, SIGNAL( collectionChanged( const Akonadi::Collection& ) ),
           SLOT( collectionChanged( const Akonadi::Collection& ) ) );
  connect( mMonitor, SIGNAL( collectionRemoved( const Akonadi::Collection& ) ),
           SLOT( collectionRemoved( const Akonadi::Collection& ) ) );

  connect( q, SIGNAL( status( int, QString ) ), q, SLOT( slotStatus( int, QString ) ) );
  connect( q, SIGNAL( percent( int ) ), q, SLOT( slotPercent( int ) ) );
  connect( q, SIGNAL( warning( QString ) ), q, SLOT( slotWarning( QString ) ) );
  connect( q, SIGNAL( error( QString ) ), q, SLOT( slotError( QString ) ) );

  // Use reference counting to allow agents to finish internal jobs when the
  // agent is stopped.
  KGlobal::ref();
  KGlobal::setAllowQuit(true);

  QTimer::singleShot( 0, q, SLOT( delayedInit() ) );
}

void AgentBasePrivate::delayedInit()
{
  Q_Q( AgentBase );
  if ( !QDBusConnection::sessionBus().registerService( QLatin1String( "org.freedesktop.Akonadi.Agent." ) + mId ) )
    kFatal() << "Unable to register service at dbus:" << QDBusConnection::sessionBus().lastError().message();
  q->setOnline( mOnline );
}

void AgentBasePrivate::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  kDebug() << "mObserver=" << (void*) mObserver << "this=" << (void*) this;
  if ( mObserver != 0 )
    mObserver->itemAdded( item, collection );
}

void AgentBasePrivate::itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers )
{
  kDebug() << "mObserver=" << (void*) mObserver << "this=" << (void*) this;
  if ( mObserver != 0 )
    mObserver->itemChanged( item, partIdentifiers );
}

void AgentBasePrivate::itemMoved( const Akonadi::Item &item, const Akonadi::Collection &source, const Akonadi::Collection &dest )
{
  kDebug() << "mObserver=" << (void*) mObserver << "this=" << (void*) this;
  if ( mObserver ) {
    // inter-resource moves, requires we know which resources the source and destination are in though
    if ( !source.resource().isEmpty() && !dest.resource().isEmpty() ) {
      if ( source.resource() != dest.resource() ) {
        if ( source.resource() == q_ptr->identifier() ) // moved away from us
          mObserver->itemRemoved( item );
        else if ( dest.resource() == q_ptr->identifier() ) // moved to us
          mObserver->itemAdded( item, dest );
        else // not for us, not sure if we should get here at all
          changeProcessed();
        return;
      }
    }
    // either incomplete information or intra-resource move
    // ### we cannot just call itemRemoved here as this will already trigger changeProcessed()
    // so, just itemAdded() is good enough as no resource can have implemented intra-resource moves anyway
    // without using Observer2
    mObserver->itemAdded( item, dest );
    // mObserver->itemRemoved( item );
  }
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

void AgentBasePrivate::changeProcessed()
{
  mMonitor->changeProcessed();
  QTimer::singleShot( 0, mMonitor, SLOT( replayNext() ) );
}

void AgentBasePrivate::slotStatus( int status, const QString &message )
{
  mStatusMessage = message;
  mStatusCode = 0;

  switch ( status ) {
    case AgentBase::Idle:
      if ( mStatusMessage.isEmpty() )
        mStatusMessage = defaultReadyMessage();

      mStatusCode = 0;
      break;
    case AgentBase::Running:
      if ( mStatusMessage.isEmpty() )
        mStatusMessage = defaultSyncingMessage();

      mStatusCode = 1;
      break;
    case AgentBase::Broken:
      if ( mStatusMessage.isEmpty() )
        mStatusMessage = defaultErrorMessage();

      mStatusCode = 2;
      break;
    default:
      Q_ASSERT( !"Unknown status passed" );
      break;
  }
}

void AgentBasePrivate::slotPercent( int progress )
{
  mProgress = progress;
}

void AgentBasePrivate::slotWarning( const QString& message )
{
  mTracer->warning( QString::fromLatin1( "AgentBase(%1)" ).arg( mId ), message );
}

void AgentBasePrivate::slotError( const QString& message )
{
  mTracer->error( QString::fromLatin1( "AgentBase(%1)" ).arg( mId ), message );
}

void AgentBasePrivate::slotNetworkStatusChange( Solid::Networking::Status stat )
{
  Q_Q( AgentBase );
  q->setOnline( stat == Solid::Networking::Connected );
}


AgentBase::AgentBase( const QString & id )
  : d_ptr( new AgentBasePrivate( this ) )
{
  sAgentBase = this;
  d_ptr->mId = id;
  d_ptr->init();
  if ( KApplication::kApplication() )
    KApplication::kApplication()->disableSessionManagement();
}

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

  QByteArray catalog;
  char *p = strrchr( argv[0], '/' );
  if ( p )
    catalog = QByteArray( p + 1 );
  else
    catalog = QByteArray( argv[0] );

  KCmdLineArgs::init( argc, argv, identifier.toLatin1(), catalog, ki18n("Akonadi Agent"),"0.1" ,
                      ki18n("Akonadi Agent") );

  KCmdLineOptions options;
  options.add("identifier <argument>", ki18n("Agent identifier"));
  KCmdLineArgs::addCmdLineOptions( options );

  return identifier;
}

// @endcond

int AgentBase::init( AgentBase *r )
{
  QApplication::setQuitOnLastWindowClosed( false );
  KGlobal::locale()->insertCatalog( QLatin1String("libakonadi") );
  int rv = kapp->exec();
  delete r;
  return rv;
}

int AgentBase::status() const
{
  Q_D( const AgentBase );

  return d->mStatusCode;
}

QString AgentBase::statusMessage() const
{
  Q_D( const AgentBase );

  return d->mStatusMessage;
}

int AgentBase::progress() const
{
  Q_D( const AgentBase );

  return d->mProgress;
}

QString AgentBase::progressMessage() const
{
  Q_D( const AgentBase );

  return d->mProgressMessage;
}

bool AgentBase::isOnline() const
{
  Q_D( const AgentBase );

  return d->mOnline;
}

void AgentBase::setNeedsNetwork( bool needsNetwork )
{
  Q_D( AgentBase );
  d->mNeedsNetwork = needsNetwork;

  if ( d->mNeedsNetwork ) {
    connect( Solid::Networking::notifier()
           , SIGNAL( statusChanged( Solid::Networking::Status ) )
           , this, SLOT( slotNetworkStatusChange( Solid::Networking::Status ) ) );
  } else {
    disconnect( Solid::Networking::notifier(), 0, 0, 0 );
    setOnline( true );
  }
}

void AgentBase::setOnline( bool state )
{
  Q_D( AgentBase );
  d->mOnline = state;
  d->mSettings->setValue( QLatin1String( "Agent/Online" ), state );
  doSetOnline( state );
  emit onlineChanged( state );
}

void AgentBase::doSetOnline( bool online )
{
  Q_UNUSED( online );
}

void AgentBase::configure( WId windowId )
{
  Q_UNUSED( windowId );
}

#ifdef Q_OS_WIN //krazy:exclude=cpp
void AgentBase::configure( qlonglong windowId )
{
  configure( reinterpret_cast<WId>( windowId ) );
}
#endif

WId AgentBase::winIdForDialogs() const
{
  bool registered = QDBusConnection::sessionBus().interface()->isServiceRegistered( QLatin1String("org.freedesktop.akonaditray") );
  if ( !registered )
    return 0;

  QDBusInterface dbus( QLatin1String("org.freedesktop.akonaditray"), QLatin1String("/Actions"),
                       QLatin1String("org.freedesktop.Akonadi.Tray") );
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
    d->mMonitor->setConfig( 0 );
    d->mSettings->sync();
  }

  KGlobal::deref();
}

void AgentBase::aboutToQuit()
{
}

void AgentBase::cleanup()
{
  Q_D( AgentBase );
  // prevent the monitor from picking up deletion signals for our own data if we are a resource
  // and thus avoid that we kill our own data as last act before our own death
  d->mMonitor->blockSignals( true );

  aboutToQuit();

  const QString fileName = d->mSettings->fileName();

  /*
   * First destroy the settings object...
   */
  d->mMonitor->setConfig( 0 );
  delete d->mSettings;
  d->mSettings = 0;

  /*
   * ... then remove the file from hd.
   */
  QFile::remove( fileName );

  /*
   * ... and also remove the agent configuration file if there is one.
   */
  QString configFile = KStandardDirs::locateLocal( "config", KGlobal::config()->name() );
  QFile::remove( configFile );

  KGlobal::deref();
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

void AgentBase::setAgentName( const QString &name )
{
  Q_D( AgentBase );
  if ( name == d->mName )
    return;

  // TODO: rename collection
  d->mName = name;

  if ( d->mName.isEmpty() || d->mName == d->mId ) {
    d->mSettings->remove( QLatin1String( "Resource/Name" ) );
    d->mSettings->remove( QLatin1String( "Agent/Name" ) );
  } else
    d->mSettings->setValue( QLatin1String( "Agent/Name" ), d->mName );

  d->mSettings->sync();

  emit agentNameChanged( d->mName );
}

QString AgentBase::agentName() const
{
  Q_D( const AgentBase );
  if ( d->mName.isEmpty() )
    return d->mId;
  else
    return d->mName;
}

void AgentBase::changeProcessed()
{
  Q_D( AgentBase );
  d->changeProcessed();
}

ChangeRecorder * AgentBase::changeRecorder() const
{
  return d_ptr->mMonitor;
}

void AgentBase::reconfigure()
{
  emit reloadConfiguration();
}

#include "agentbase.moc"
#include "agentbase_p.moc"
