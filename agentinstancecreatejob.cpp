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

#include <kdebug.h>
#include <klocale.h>

#include <QTimer>

using namespace Akonadi;

static const int safetyTimeout = 10000; // ms

class AgentInstanceCreateJob::Private
{
  public:
    Private( AgentInstanceCreateJob* parent ) : q( parent ),
      manager( 0 ),
      windowId( 0 ),
      safetyTimer( 0 ),
      doConfig( false ),
      tooLate( false )
    {
    }

    ~Private()
    {
      delete manager;
    }

    void agentInstanceAdded( const QString &id )
    {
      if ( instanceId == id && !tooLate ) {
        safetyTimer->stop();
        if ( doConfig ) {
          // return from dbus call first before doing the next one
          QTimer::singleShot( 0, q, SLOT(doConfigure()) );
        } else {
          q->emitResult();
        }
      }
    }

    void doConfigure()
    {
      manager->agentInstanceConfigure( instanceId, windowId );
      q->emitResult();
    }

    void timeout()
    {
      tooLate = true;
      q->setError( KJob::UserDefinedError );
      q->setErrorText( i18n( "Agent instance creation timed out." ) );
      q->emitResult();
    }

    AgentInstanceCreateJob* q;
    QString typeIdentifier;
    QString instanceId;
    AgentManager *manager;
    WId windowId;
    QTimer *safetyTimer;
    bool doConfig;
    bool tooLate;
};

AgentInstanceCreateJob::AgentInstanceCreateJob(const QString & typeIdentifier, QObject * parent) :
    KJob( parent ),
    d( new Private( this ) )
{
  d->typeIdentifier = typeIdentifier;
  d->manager = new AgentManager( this );
  connect( d->manager, SIGNAL(agentInstanceAdded(QString)), SLOT(agentInstanceAdded(QString)) );
  d->safetyTimer = new QTimer( this );
  connect( d->safetyTimer, SIGNAL(timeout()), SLOT(timeout()) );
  d->safetyTimer->start( safetyTimeout );
}

AgentInstanceCreateJob::~ AgentInstanceCreateJob()
{
  delete d;
}

void AgentInstanceCreateJob::configure( WId windowId )
{
  d->windowId = windowId;
  d->doConfig = true;
}

void AgentInstanceCreateJob::start()
{
  d->instanceId = d->manager->createAgentInstance( d->typeIdentifier );
  if ( d->instanceId.isEmpty() ) {
    setError( KJob::UserDefinedError );
    setErrorText( i18n("Unable to create agent instance." ) );
    emitResult();
  }
}

#include "agentinstancecreatejob.moc"
