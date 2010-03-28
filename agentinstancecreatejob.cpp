/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "agentinstancecreatejob.h"

#include "agentinstance.h"
#include "agentmanager.h"
#include "agentmanager_p.h"
#include "controlinterface.h"
#include "kjobprivatebase_p.h"

#include <kdebug.h>
#include <klocale.h>

#include <QtCore/QTimer>

#ifdef Q_OS_UNIX
#include <sys/types.h>
#include <signal.h>
#endif

using namespace Akonadi;

static const int safetyTimeout = 10000; // ms

/**
 * @internal
 */
class AgentInstanceCreateJob::Private : public KJobPrivateBase
{
  public:
    Private( AgentInstanceCreateJob* parent ) : q( parent ),
      parentWidget( 0 ),
      safetyTimer( new QTimer( parent ) ),
      doConfig( false ),
      tooLate( false )
    {
      QObject::connect( AgentManager::self(), SIGNAL( instanceAdded( const Akonadi::AgentInstance& ) ),
                        q, SLOT( agentInstanceAdded( const Akonadi::AgentInstance& ) ) );
      QObject::connect( safetyTimer, SIGNAL( timeout() ), q, SLOT( timeout() ) );
    }

    void agentInstanceAdded( const AgentInstance &instance )
    {
      if ( agentInstance == instance && !tooLate ) {
        safetyTimer->stop();
        if ( doConfig ) {
          // return from dbus call first before doing the next one
          QTimer::singleShot( 0, q, SLOT( doConfigure() ) );
        } else {
          q->emitResult();
        }
      }
    }

    void doConfigure()
    {
      org::freedesktop::Akonadi::Agent::Control *agentControlIface =
        new org::freedesktop::Akonadi::Agent::Control( QLatin1String( "org.freedesktop.Akonadi.Agent." ) + agentInstance.identifier(),
                                                       QLatin1String( "/" ), QDBusConnection::sessionBus(), q );
      if ( !agentControlIface || !agentControlIface->isValid() ) {
        if ( agentControlIface )
          delete agentControlIface;

        q->setError( KJob::UserDefinedError );
        q->setErrorText( i18n( "Unable to access dbus interface of created agent." ) );
        q->emitResult();
        return;
      }

      q->connect( agentControlIface, SIGNAL( configurationDialogAccepted() ),
                  q, SLOT( configurationDialogAccepted() ) );
      q->connect( agentControlIface, SIGNAL( configurationDialogRejected() ),
                  q, SLOT( configurationDialogRejected() ) );

      agentInstance.configure( parentWidget );
    }

    void configurationDialogAccepted()
    {
      // The user clicked 'Ok' in the initial configuration dialog, so we assume
      // he wants to keep the resource and the job is done.
      q->emitResult();
    }

    void configurationDialogRejected()
    {
      // The user clicked 'Cancel' in the initial configuration dialog, so we assume
      // he wants to abort the 'create new resource' job and the new resource will be
      // removed again.
      AgentManager::self()->removeInstance( agentInstance );

      q->emitResult();
    }

    void timeout()
    {
      tooLate = true;
      q->setError( KJob::UserDefinedError );
      q->setErrorText( i18n( "Agent instance creation timed out." ) );
      q->emitResult();
    }

    void emitResult()
    {
      q->emitResult();
    }

    void doStart();

    AgentInstanceCreateJob* q;
    AgentType agentType;
    QString agentTypeId;
    AgentInstance agentInstance;
    QWidget* parentWidget;
    QTimer *safetyTimer;
    bool doConfig;
    bool tooLate;
};

AgentInstanceCreateJob::AgentInstanceCreateJob( const AgentType &agentType, QObject *parent )
  : KJob( parent ),
    d( new Private( this ) )
{
  d->agentType = agentType;
}

AgentInstanceCreateJob::AgentInstanceCreateJob( const QString &typeId, QObject *parent )
  : KJob( parent ),
    d( new Private( this ) )
{
  d->agentTypeId = typeId;
}

AgentInstanceCreateJob::~ AgentInstanceCreateJob()
{
  delete d;
}

void AgentInstanceCreateJob::configure( QWidget *parent )
{
  d->parentWidget = parent;
  d->doConfig = true;
}

AgentInstance AgentInstanceCreateJob::instance() const
{
  return d->agentInstance;
}

void AgentInstanceCreateJob::start()
{
  d->start();
}

void AgentInstanceCreateJob::Private::doStart()
{
  if ( !agentType.isValid() && !agentTypeId.isEmpty() )
    agentType = AgentManager::self()->type( agentTypeId );

  if ( !agentType.isValid() ) {
    q->setError( KJob::UserDefinedError );
    q->setErrorText( i18n( "Unable to obtain agent type '%1'.", agentTypeId) );
    QTimer::singleShot( 0, q, SLOT( emitResult() ) );
    return;
  }

  agentInstance = AgentManager::self()->d->createInstance( agentType );
  if ( !agentInstance.isValid() ) {
    q->setError( KJob::UserDefinedError );
    q->setErrorText( i18n( "Unable to create agent instance." ) );
    QTimer::singleShot( 0, q, SLOT( emitResult() ) );
  } else {
    int timeout = safetyTimeout;
#ifdef Q_OS_UNIX
    // Increate the timeout when valgrinding the agent, because that slows down things a log.
    QString agentValgrind = QString::fromLocal8Bit( qgetenv( "AKONADI_VALGRIND" ) );
    if ( !agentValgrind.isEmpty() && agentType.identifier().contains( agentValgrind ) )
      timeout *= 15;
#endif
    safetyTimer->start( timeout );
  }
}

#include "agentinstancecreatejob.moc"
