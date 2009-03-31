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

#include "agentinstance.h"

#include "agenttype.h"
#include "agentmanager.h"
#include "../../libs/xdgbasedirs_p.h"
#include "processcontrol.h"

AgentInstance::AgentInstance(AgentManager * manager) :
    QObject( manager ),
    mManager( manager ),
    mController( 0 ),
    mAgentControlInterface( 0 ),
    mAgentStatusInterface( 0 ),
    mResourceInterface( 0 ),
    mStatus( 0 ),
    mPercent( 0 ),
    mOnline( false )
{
}

bool AgentInstance::start( const AgentType &agentInfo )
{
  Q_ASSERT( !mIdentifier.isEmpty() );
  if ( mIdentifier.isEmpty() )
    return false;
  mType = agentInfo.identifier;
  const QString executable = Akonadi::XdgBaseDirs::findExecutableFile( agentInfo.exec );
  if ( executable.isEmpty() ) {
    mManager->tracer()->error( QLatin1String( "AgentInstanceInfo::start" ),
                               QString::fromLatin1( "Unable to find agent executable '%1'" ).arg( agentInfo.exec ) );
    return false;
  }
  mController = new Akonadi::ProcessControl( this );
  QStringList arguments;
  arguments << "--identifier" << mIdentifier;
  mController->start( executable, arguments );
  return true;
}

void AgentInstance::quit()
{
  mController->setCrashPolicy( Akonadi::ProcessControl::StopOnCrash );
  if ( mAgentControlInterface && mAgentControlInterface->isValid() )
    mAgentControlInterface->quit();
}

void AgentInstance::cleanup()
{
  mController->setCrashPolicy( Akonadi::ProcessControl::StopOnCrash );
  if ( mAgentControlInterface && mAgentControlInterface->isValid() )
    mAgentControlInterface->cleanup();
}

bool AgentInstance::obtainAgentInterface()
{
  delete mAgentControlInterface;
  delete mAgentStatusInterface;
  mAgentControlInterface = 0;
  mAgentStatusInterface = 0;

  org::freedesktop::Akonadi::Agent::Control *agentControlIface =
    new org::freedesktop::Akonadi::Agent::Control( "org.freedesktop.Akonadi.Agent." + mIdentifier,
                                                   "/", QDBusConnection::sessionBus(), this );
  if ( !agentControlIface || !agentControlIface->isValid() ) {
    mManager->tracer()->error( QLatin1String( "AgentInstance::obtainAgentInterface" ),
      QString( "Cannot connect to agent instance with identifier '%1', error message: '%2'" )
          .arg( mIdentifier, agentControlIface ? agentControlIface->lastError().message() : "" ) );
    return false;
  }
  mAgentControlInterface = agentControlIface;

  org::freedesktop::Akonadi::Agent::Status *agentStatusIface =
    new org::freedesktop::Akonadi::Agent::Status( "org.freedesktop.Akonadi.Agent." + mIdentifier,
                                                  "/", QDBusConnection::sessionBus(), this );
  if ( !agentStatusIface || !agentStatusIface->isValid() ) {
    mManager->tracer()->error( QLatin1String( "AgentInstance::obtainAgentInterface" ),
      QString( "Cannot connect to agent instance with identifier '%1', error message: '%2'" )
          .arg( mIdentifier, agentStatusIface ? agentStatusIface->lastError().message() : "" ) );
    return false;
  }
  mAgentStatusInterface = agentStatusIface;

  connect( agentStatusIface, SIGNAL(status(int,QString)), SLOT(statusChanged(int,QString)) );
  connect( agentStatusIface, SIGNAL(percent(int)), SLOT(percentChanged(int)) );
  connect( agentStatusIface, SIGNAL(warning(QString)), SLOT(warning(QString)) );
  connect( agentStatusIface, SIGNAL(error(QString)), SLOT(error(QString)) );
  connect( agentStatusIface, SIGNAL(onlineChanged(bool)), SLOT(onlineChanged(bool)) );

  refreshAgentStatus();
  return true;
}

bool AgentInstance::obtainResourceInterface()
{
  delete mResourceInterface;
  mResourceInterface = 0;

  org::freedesktop::Akonadi::Resource *resInterface =
    new org::freedesktop::Akonadi::Resource( "org.freedesktop.Akonadi.Resource." + mIdentifier,
                                              "/", QDBusConnection::sessionBus(), this );

  if ( !resInterface || !resInterface->isValid() ) {
    mManager->tracer()->error( QLatin1String( "AgentInstance::obtainResourceInterface" ),
      QString( "Cannot connect to agent instance with identifier '%1', error message: '%2'" )
          .arg( mIdentifier, resInterface ? resInterface->lastError().message() : "" ) );
    return false;
  }

  connect( resInterface, SIGNAL(nameChanged(QString)), SLOT(resourceNameChanged(QString)) );
  mResourceInterface = resInterface;

  refreshResourceStatus();
  return true;
}

void AgentInstance::statusChanged(int status, const QString & statusMsg)
{
  if ( mStatus == status && mStatusMessage == statusMsg )
    return;
  mStatus = status;
  mStatusMessage = statusMsg;
  emit mManager->agentInstanceStatusChanged( mIdentifier, mStatus, mStatusMessage );
}

void AgentInstance::statusStateChanged(int status)
{
  statusChanged( status, mStatusMessage );
}

void AgentInstance::statusMessageChanged(const QString & msg)
{
  statusChanged( mStatus, msg );
}

void AgentInstance::percentChanged(int percent)
{
  if ( mPercent == percent )
    return;
  mPercent = percent;
  emit mManager->agentInstanceProgressChanged( mIdentifier, mPercent, QString() );
}

void AgentInstance::warning(const QString & msg)
{
  emit mManager->agentInstanceWarning( mIdentifier, msg );
}

void AgentInstance::error(const QString & msg)
{
  emit mManager->agentInstanceError( mIdentifier, msg );
}

void AgentInstance::onlineChanged(bool state)
{
  if ( mOnline == state )
    return;
  mOnline = state;
  emit mManager->agentInstanceOnlineChanged( mIdentifier, state );
}

void AgentInstance::resourceNameChanged(const QString & name)
{
  if ( name == mResourceName )
    return;
  mResourceName = name;
  emit mManager->agentInstanceNameChanged( mIdentifier, name );
}

void AgentInstance::refreshAgentStatus()
{
  if ( !hasAgentInterface() )
    return;

  // async calls so we are not blocked by misbehaving agents
  mAgentStatusInterface->callWithCallback( QLatin1String("status"), QList<QVariant>(),
                                           this, SLOT(statusStateChanged(int)),
                                           SLOT(errorHandler(QDBusError)) );
  mAgentStatusInterface->callWithCallback( QLatin1String("statusMessage"), QList<QVariant>(),
                                           this, SLOT(statusMessageChanged(QString)),
                                           SLOT(errorHandler(QDBusError)) );
  mAgentStatusInterface->callWithCallback( QLatin1String("progress"), QList<QVariant>(),
                                           this, SLOT(percentChanged(int)),
                                           SLOT(errorHandler(QDBusError)) );
  mAgentStatusInterface->callWithCallback( QLatin1String("isOnline"), QList<QVariant>(),
                                           this, SLOT(onlineChanged(bool)),
                                           SLOT(errorHandler(QDBusError)) );
}

void AgentInstance::refreshResourceStatus()
{
  if ( !hasResourceInterface() )
    return;

  // async call so we are not blocked by misbehaving resources
  mResourceInterface->callWithCallback( QLatin1String("name"), QList<QVariant>(),
                                        this, SLOT(resourceNameChanged(QString)),
                                        SLOT(errorHandler(QDBusError)) );
}

void AgentInstance::errorHandler(const QDBusError & error)
{
  //avoid using the server tracer, can result in D-BUS lockups
  qDebug() <<  QString( "D-Bus communication error '%1': '%2'" ).arg( error.name(), error.message() ) ;
  // TODO try again after some time, esp. on timeout errors
}

void AgentInstance::restartWhenIdle()
{
  if ( mStatus == 0 ) {
    mController->restartOnceWhenFinished();
    quit();
  }
}

#include "agentinstance.moc"
