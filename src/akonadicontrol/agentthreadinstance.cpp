/*
    Copyright (c) 2010 Bertjan Broeksema <broeksema@kde.org>

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
#include "agentthreadinstance.h"

#include "agentserverinterface.h"
#include "agenttype.h"
#include "akonadicontrol_debug.h"

#include <private/dbus_p.h>

#include <QDBusServiceWatcher>

using namespace Akonadi;

AgentThreadInstance::AgentThreadInstance(AgentManager *manager)
    : AgentInstance(manager)
{
    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(Akonadi::DBus::serviceName(Akonadi::DBus::AgentServer),
                                                           QDBusConnection::sessionBus(),
                                                           QDBusServiceWatcher::WatchForRegistration, this);
    connect(watcher, &QDBusServiceWatcher::serviceRegistered,
            this, &AgentThreadInstance::agentServerRegistered);
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
                                                       QStringLiteral("/AgentServer"), QDBusConnection::sessionBus());
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
                                                       QStringLiteral("/AgentServer"), QDBusConnection::sessionBus());
    agentServer.stopAgent(identifier());
}

void AgentThreadInstance::restartWhenIdle()
{
    if (status() != 1 && !identifier().isEmpty()) {
        org::freedesktop::Akonadi::AgentServer agentServer(Akonadi::DBus::serviceName(Akonadi::DBus::AgentServer),
                                                           QStringLiteral("/AgentServer"), QDBusConnection::sessionBus());
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
                                                       QStringLiteral("/AgentServer"), QDBusConnection::sessionBus());
    agentServer.agentInstanceConfigure(identifier(), windowId);
}
