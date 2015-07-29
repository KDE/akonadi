/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>

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
#include "agentinstance_p.h"

#include "agentmanager.h"
#include "agentmanager_p.h"
#include "servermanager.h"

#include <QDebug>

using namespace Akonadi;

AgentInstance::AgentInstance()
    : d(new Private)
{
}

AgentInstance::AgentInstance(const AgentInstance &other)
    : d(other.d)
{
}

AgentInstance::~AgentInstance()
{
}

bool AgentInstance::isValid() const
{
    return !d->mIdentifier.isEmpty();
}

AgentType AgentInstance::type() const
{
    return d->mType;
}

QString AgentInstance::identifier() const
{
    return d->mIdentifier;
}

void AgentInstance::setName(const QString &name)
{
    AgentManager::self()->d->setName(*this, name);
}

QString AgentInstance::name() const
{
    return d->mName;
}

AgentInstance::Status AgentInstance::status() const
{
    switch (d->mStatus) {
    case 0:
        return Idle;
    case 1:
        return Running;
    case 2:
    default:
        return Broken;
    case 3:
        return NotConfigured;
    }
}

QString AgentInstance::statusMessage() const
{
    return d->mStatusMessage;
}

int AgentInstance::progress() const
{
    return d->mProgress;
}

bool AgentInstance::isOnline() const
{
    return d->mIsOnline;
}

void AgentInstance::setIsOnline(bool online)
{
    AgentManager::self()->d->setOnline(*this, online);
}

void AgentInstance::configure(QWidget *parent)
{
    AgentManager::self()->d->configure(*this, parent);
}

void AgentInstance::synchronize()
{
    AgentManager::self()->d->synchronize(*this);
}

void AgentInstance::synchronizeCollectionTree()
{
    AgentManager::self()->d->synchronizeCollectionTree(*this);
}

AgentInstance &AgentInstance::operator=(const AgentInstance &other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

bool AgentInstance::operator==(const AgentInstance &other) const
{
    return (d->mIdentifier == other.d->mIdentifier);
}

void AgentInstance::abortCurrentTask() const
{
    QDBusInterface iface(ServerManager::agentServiceName(ServerManager::Agent, identifier()),
                         QStringLiteral("/"),
                         QStringLiteral("org.freedesktop.Akonadi.Agent.Control"));
    if (iface.isValid()) {
        QDBusReply<void> reply = iface.call(QStringLiteral("abort"));
        if (!reply.isValid()) {
            qWarning() << "Failed to place D-Bus call.";
        }
    } else {
        qWarning() << "Unable to obtain agent interface";
    }
}

void AgentInstance::reconfigure() const
{
    QDBusInterface iface(ServerManager::agentServiceName(ServerManager::Agent, identifier()),
                         QStringLiteral("/"),
                         QStringLiteral("org.freedesktop.Akonadi.Agent.Control"));
    if (iface.isValid()) {
        QDBusReply<void> reply = iface.call(QStringLiteral("reconfigure"));
        if (!reply.isValid()) {
            qWarning() << "Failed to place D-Bus call.";
        }
    } else {
        qWarning() << "Unable to obtain agent interface";
    }
}

void Akonadi::AgentInstance::restart() const
{
    QDBusInterface iface(ServerManager::serviceName(Akonadi::ServerManager::Control),
                         QStringLiteral("/AgentManager"),
                         QStringLiteral("org.freedesktop.Akonadi.AgentManager"));
    if (iface.isValid()) {
        QDBusReply<void> reply = iface.call(QStringLiteral("restartAgentInstance"), identifier());
        if (!reply.isValid()) {
            qWarning() << "Failed to place D-Bus call.";
        }
    } else {
        qWarning() << "Unable to obtain control interface" << iface.lastError().message();
    }
}
