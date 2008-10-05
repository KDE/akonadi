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
#include "agentmanager.h"
#include "agentmanager_p.h"

#include "agentinstance.h"

#include <kdebug.h>
#include <klocale.h>

#include <QtCore/QTimer>

using namespace Akonadi;

static const int safetyTimeout = 10000; // ms

/**
 * @internal
 */
class AgentInstanceCreateJob::Private
{
  public:
    Private( AgentInstanceCreateJob* parent ) : q( parent ),
      parentWidget( 0 ),
      safetyTimer( 0 ),
      doConfig( false ),
      tooLate( false )
    {
    }

    ~Private()
    {
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
      agentInstance.configure( parentWidget );
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

    AgentInstanceCreateJob* q;
    AgentType agentType;
    AgentInstance agentInstance;
    QWidget* parentWidget;
    QTimer *safetyTimer;
    bool doConfig;
    bool tooLate;
};

AgentInstanceCreateJob::AgentInstanceCreateJob(const AgentType & agentType, QObject * parent) :
    KJob( parent ),
    d( new Private( this ) )
{
  d->agentType = agentType;
  connect( AgentManager::self(), SIGNAL(instanceAdded(const Akonadi::AgentInstance&)),
           this, SLOT(agentInstanceAdded(const Akonadi::AgentInstance&)) );

  d->safetyTimer = new QTimer( this );
  connect( d->safetyTimer, SIGNAL(timeout()), SLOT(timeout()) );
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
  d->agentInstance = AgentManager::self()->d->createInstance( d->agentType );
  if ( !d->agentInstance.isValid() ) {
    setError( KJob::UserDefinedError );
    setErrorText( i18n("Unable to create agent instance." ) );
    QTimer::singleShot( 0, this , SLOT(emitResult()) );
  } else {
    d->safetyTimer->start( safetyTimeout );
  }
}

#include "agentinstancecreatejob.moc"
