/*
    This file is part of libakonadi.

    Copyright (c) 2006 Till Adam <adam@kde.org>
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2007 Bruno Virlet <bruno.virlet@gmail.com>

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
#include "kcrash.h"
#include "agentadaptor.h"
#include "monitor_p.h"
#include "xdgbasedirs.h"

#include <libakonadi/session.h>
#include <libakonadi/changerecorder.h>
#include <libakonadi/itemfetchjob.h>

#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtDBus/QtDBus>

#include <signal.h>
#include <stdlib.h>

using namespace Akonadi;

static AgentBase *sAgentBase = 0;

/* FIXME Already defined in ResourceBase: which one to keep ?
void crashHandler( int signal )
{
  if ( sAgentBase )
    sAgentBase->crashHandler( signal );

  exit( 255 );
}*/

AgentBasePrivate::AgentBasePrivate( AgentBase *parent )
  : q_ptr( parent ),
    mSettings( 0 )
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

  if ( !QDBusConnection::sessionBus().registerService( QLatin1String( "org.kde.Akonadi.Agent." ) + mId ) )
    q->error( QString::fromLatin1( "Unable to register service at dbus: %1" ).arg( QDBusConnection::sessionBus().lastError().message() ) );

  new AgentAdaptor( q );
  if ( !QDBusConnection::sessionBus().registerObject( QLatin1String( "/" ), q, QDBusConnection::ExportAdaptors ) )
    q->error( QString::fromLatin1( "Unable to register object at dbus: %1" ).arg( QDBusConnection::sessionBus().lastError().message() ) );

  mSettings = new QSettings( QString::fromLatin1( "%1/agent_config_%2" ).arg( XdgBaseDirs::saveDir( "config", QLatin1String( "akonadi" ) ), mId ), QSettings::IniFormat );

  session = new Session( mId.toLatin1(), q );
  monitor = new ChangeRecorder( q );
  monitor->ignoreSession( session );
  monitor->setConfig( mSettings );

  q->connect( monitor, SIGNAL( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ),
              q, SLOT( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ) );
  q->connect( monitor, SIGNAL( itemChanged( const Akonadi::Item&, const QStringList& ) ),
              q, SLOT( itemChanged( const Akonadi::Item&, const QStringList& ) ) );
  q->connect( monitor, SIGNAL( itemRemoved( const Akonadi::DataReference& ) ),
           q, SLOT( itemRemoved( const Akonadi::DataReference& ) ) );
  q->connect( monitor, SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)),
              q, SLOT(collectionAdded(Akonadi::Collection,Akonadi::Collection)) );
  q->connect( monitor, SIGNAL( collectionChanged( const Akonadi::Collection& ) ),
              q, SLOT( collectionChanged( const Akonadi::Collection& ) ) );
  q->connect( monitor, SIGNAL( collectionRemoved( int, const QString& ) ),
              q, SLOT( collectionRemoved( int, const QString& ) ) );

  // initial configuration
  bool initialized = mSettings->value( QLatin1String( "Agent/Initialized" ), false ).toBool();
  if ( !initialized ) {
    QTimer::singleShot( 0, q, SLOT(configure()) ); // finish construction first
    mSettings->setValue( QLatin1String( "Agent/Initialized" ), true );
  }
}


AgentBase::AgentBase( const QString & id )
  : d_ptr( new AgentBasePrivate( this ) )
{
  KCrash::init();
  // FIXME See above KCrash::setEmergencyMethod( ::crashHandler );
  sAgentBase = this;
  d_ptr->mId = id;
  d_ptr->init();
}

AgentBase::AgentBase( AgentBasePrivate* d, const QString &id ) :
    d_ptr( d )
{
  KCrash::init();
  // FIXME See above KCrash::setEmergencyMethod( ::crashHandler );
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
    qDebug( "AgentBase::parseArguments: Not enough arguments passed..." );
    exit( 1 );
  }

  for ( int i = 1; i < argc - 1; ++i ) {
    if ( QLatin1String( argv[ i ] ) == QLatin1String( "--identifier" ) )
      identifier = QLatin1String( argv[ i + 1 ] );
  }

  if ( identifier.isEmpty() ) {
    qDebug( "AgentBase::parseArguments: Identifier argument missing" );
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

void AgentBase::configure()
{
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

  QCoreApplication::exit( 0 );
}

void AgentBase::crashHandler( int signal )
{
  /**
   * If we retrieved a SIGINT or SIGTERM we close normally
   */
  if ( signal == SIGINT || signal == SIGTERM )
    quit();
}

QString AgentBase::identifier() const
{
  return d_ptr->mId;
}

void AgentBase::itemAdded( const Item &item, const Collection &collection )
{
  Q_UNUSED( item );
  Q_UNUSED( collection );
}

void AgentBase::itemChanged( const Item &item, const QStringList &partIdentifiers )
{
  Q_UNUSED( item );
  Q_UNUSED( partIdentifiers );
}

void AgentBase::itemRemoved( const DataReference &ref )
{
  Q_UNUSED( ref );
}

void AgentBase::collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent )
{
  Q_UNUSED( collection );
}

void AgentBase::collectionChanged( const Collection &collection )
{
  Q_UNUSED( collection );
}

void AgentBase::collectionRemoved( int id, const QString &remoteId )
{
  Q_UNUSED( id );
  Q_UNUSED( remoteId );
}

QSettings* AgentBase::settings()
{
  return d_ptr->mSettings;
}

Session* AgentBase::session()
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

ChangeRecorder * AgentBase::monitor() const
{
  return d_ptr->monitor;
}

#include "agent.moc"
#include "agentbase.moc"
