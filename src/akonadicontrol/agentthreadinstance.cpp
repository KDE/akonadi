/*
    SPDX-FileCopyrightText: 2010 Bertjan Broeksema <broeksema@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "agentthreadinstance.h"

#include "agentserverinterface.h"
#include "akonadicontrol_debug.h"

#include "private/dbus_p.h"

using namespace Akonadi;

AgentThreadInstance::AgentThreadInstance(AgentManager &manager)
    : AgentInstance(manager)
    , mServiceWatcher(Akonadi::DBus::serviceName(Akonadi::DBus::AgentServer), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForRegistration)
{
    connect(&mServiceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &AgentThreadInstance::agentServerRegistered);
}

bool AgentThreadInstance::start(const AgentType &agentInfo)
{
    Q_ASSERT(!identifier().isEmpty());
    if (identifier().isEmpty()) {
        return false;
    }

    setAgentType(agentInfo.identifier);
    mAgentType = agentInfo;

    org::freedesktop::Akonadi::AgentServer agentServer(Akonadi::DBus::serviceName(Akonadi::DBus::AgentServer),
                                                       QStringLiteral("/AgentServer"),
                                                       QDBusConnection::sessionBus());
    if (!agentServer.isValid()) {
        qCDebug(AKONADICONTROL_LOG) << "AgentServer not up (yet?)";
        return false;
    }

    // TODO: let startAgent return a bool.
    agentServer.startAgent(identifier(), agentInfo.identifier, agentInfo.exec);
    return true;
}

void AgentThreadInstance::quit()
{
    AgentInstance::quit();

    org::freedesktop::Akonadi::AgentServer agentServer(Akonadi::DBus::serviceName(Akonadi::DBus::AgentServer),
                                                       QStringLiteral("/AgentServer"),
                                                       QDBusConnection::sessionBus());
    agentServer.stopAgent(identifier());
}

void AgentThreadInstance::restartWhenIdle()
{
    if (status() != 1 && !identifier().isEmpty()) {
        org::freedesktop::Akonadi::AgentServer agentServer(Akonadi::DBus::serviceName(Akonadi::DBus::AgentServer),
                                                           QStringLiteral("/AgentServer"),
                                                           QDBusConnection::sessionBus());
        agentServer.stopAgent(identifier());
        agentServer.startAgent(identifier(), agentType(), mAgentType.exec);
    }
}

void AgentThreadInstance::agentServerRegistered()
{
    start(mAgentType);
}

void Akonadi::AgentThreadInstance::configure(qlonglong windowId)
{
    org::freedesktop::Akonadi::AgentServer agentServer(Akonadi::DBus::serviceName(Akonadi::DBus::AgentServer),
                                                       QStringLiteral("/AgentServer"),
                                                       QDBusConnection::sessionBus());
    agentServer.agentInstanceConfigure(identifier(), windowId);
}

#include "moc_agentthreadinstance.cpp"
