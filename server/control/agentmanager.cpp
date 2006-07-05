
#include "agentmanager.h"

#include "agentmanageradaptor.h"

AgentManager::AgentManager( QObject *parent )
  : QObject( parent )
{
  new AgentManagerAdaptor( this );

  QDBus::sessionBus().registerObject( "/", this );
}

AgentManager::~AgentManager()
{
}

QStringList AgentManager::agentTypes() const
{
  return QStringList();
}

QString AgentManager::agentName( const QString &identifier ) const
{
  return QString();
}

QString AgentManager::agentComment( const QString &identifier ) const
{
  return QString();
}

QString AgentManager::agentIcon( const QString &identifier ) const
{
  return QString();
}

QStringList AgentManager::agentMimeTypes( const QString &identifier ) const
{
  return QStringList();
}

QStringList AgentManager::agentCapabilities( const QString &identifier ) const
{
  return QStringList();
}

QString AgentManager::createAgentInstance( const QString &identifier )
{
  return QString();
}

void AgentManager::removeAgentInstance( const QString &identifier )
{
}

QStringList AgentManager::profiles() const
{
  return QStringList();
}

bool AgentManager::createProfile( const QString &identifier )
{
}

void AgentManager::removeProfile( const QString &identifier )
{
}

void AgentManager::profileAddAgent( const QString &profileIdentifier, const QString &agentIdentifier )
{
}

void AgentManager::profileRemoveAgent( const QString &profileIdentifier, const QString &agentIdentifier )
{
}

QStringList AgentManager::profileAgents( const QString &identifier ) const
{
  return QStringList();
}

#include "agentmanager.moc"
