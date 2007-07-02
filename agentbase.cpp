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

#include "kcrash.h"
#include "agentadaptor.h"
#include "monitor_p.h"
#include "tracerinterface.h"

#include <libakonadi/session.h>
#include <libakonadi/monitor.h>
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

class AgentBase::Private
{
  public:
    Private( AgentBase *parent )
      : mParent( parent ),
        mSettings( 0 )
    {
    }

    void slotItemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void slotItemChanged( const Akonadi::Item &item, const QStringList& );
    void slotItemRemoved( const Akonadi::DataReference &reference );
    void slotCollectionAdded( const Akonadi::Collection &collection );
    void slotCollectionChanged( const Akonadi::Collection &collection );
    void slotCollectionRemoved( int id, const QString &remoteId );

    QString defaultErrorMessage() const;

    AgentBase *mParent;

    QString mId;
    QString mName;

    QSettings *mSettings;

    Session *session;
    Monitor *monitor;

    org::kde::Akonadi::Tracer *mTracer;
};

QString AgentBase::Private::defaultErrorMessage() const
{
  return tr( "Error!" );
}

void AgentBase::Private::slotItemAdded( const Akonadi::Item &item, const Collection &collection )
{
  mParent->itemAdded( item, collection );
}

void AgentBase::Private::slotItemChanged( const Akonadi::Item &item, const QStringList &partIdentifiers )
{
  mParent->itemChanged( item, partIdentifiers );
}

void AgentBase::Private::slotItemRemoved(const Akonadi::DataReference & ref)
{
  mParent->itemRemoved( ref );
}

void AgentBase::Private::slotCollectionAdded( const Akonadi::Collection &collection )
{
  mParent->collectionAdded( collection );
}

void AgentBase::Private::slotCollectionChanged( const Akonadi::Collection &collection )
{
  mParent->collectionChanged( collection );
}

void AgentBase::Private::slotCollectionRemoved( int id, const QString &remoteId )
{
  mParent->collectionRemoved( id, remoteId );
}






AgentBase::AgentBase( const QString & id )
  : d( new Private( this ) )
{
  KCrash::init();
  // FIXME See above KCrash::setEmergencyMethod( ::crashHandler );
  sAgentBase = this;

  d->mTracer = new org::kde::Akonadi::Tracer( QLatin1String( "org.kde.Akonadi" ), QLatin1String( "/tracing" ),
                                              QDBusConnection::sessionBus(), this );

  if ( !QDBusConnection::sessionBus().registerService( QLatin1String( "org.kde.Akonadi.Agent." ) + id ) )
    error( QString::fromLatin1( "Unable to register service at dbus: %1" ).arg( QDBusConnection::sessionBus().lastError().message() ) );

  new AgentAdaptor( this );
  if ( !QDBusConnection::sessionBus().registerObject( QLatin1String( "/" ), this, QDBusConnection::ExportAdaptors ) )
    error( QString::fromLatin1( "Unable to register object at dbus: %1" ).arg( QDBusConnection::sessionBus().lastError().message() ) );

  d->mId = id;

  d->mSettings = new QSettings( QString::fromLatin1( "%1/.akonadi/agent_config_%2" ).arg( QDir::homePath(), id ), QSettings::IniFormat );

  const QString name = d->mSettings->value( QLatin1String( "Agent/Name" ) ).toString();
  if ( !name.isEmpty() )
    d->mName = name;

  d->session = new Session( d->mId.toLatin1(), this );
  d->monitor = new Monitor( this );
  d->monitor->fetchCollection( true );
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
  // TODO An agent should not monitor *all* by default
  d->monitor->monitorAll(); 

  // initial configuration
  bool initialized = settings()->value( QLatin1String( "Agent/Initialized" ), false ).toBool();
  if ( !initialized ) {
    QTimer::singleShot( 0, this, SLOT(configure()) ); // finish construction first
    settings()->setValue( QLatin1String( "Agent/Initialized" ), true );
  }
}

AgentBase::~AgentBase()
{
  delete d->mSettings;
  delete d;
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
  aboutToQuit();

  d->mSettings->sync();

  QCoreApplication::exit( 0 );
}

void AgentBase::aboutToQuit()
{
}

void AgentBase::cleanup() const
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
  return d->mId;
}

void AgentBase::setName( const QString &name )
{
  if ( name == d->mName )
    return;

  // TODO: rename collection
  d->mName = name;

  if ( d->mName.isEmpty() || d->mName == d->mId )
    d->mSettings->remove( QLatin1String( "Agent/Name" ) );
  else
    d->mSettings->setValue( QLatin1String( "Agent/Name" ), d->mName );

  d->mSettings->sync();

  emit nameChanged( d->mName );
}

QString AgentBase::name() const
{
  if ( d->mName.isEmpty() )
    return d->mId;
  else
    return d->mName;
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

QSettings* AgentBase::settings()
{
  return d->mSettings;
}

Session* AgentBase::session()
{
  return d->session;
}

void AgentBase::warning( const QString& message )
{
  d->mTracer->warning( QString::fromLatin1( "ResourceBase(%1)" ).arg( d->mId ), message );
}

void AgentBase::error( const QString& message )
{
  d->mTracer->error( QString::fromLatin1( "ResourceBase(%1)" ).arg( d->mId ), message );
}


#include "agent.moc"
#include "agentbase.moc"
