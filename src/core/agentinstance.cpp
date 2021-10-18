/*
    SPDX-FileCopyrightText: 2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentinstance.h"
#include "agentinstance_p.h"

#include "agentmanager.h"
#include "agentmanager_p.h"
#include "servermanager.h"

#include "akonadicore_debug.h"

using namespace Akonadi;

AgentInstance::AgentInstance()
    : d(new AgentInstancePrivate)
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
    case 3:
        return NotConfigured;
    case 2:
    default:
        return Broken;
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

void AgentInstance::synchronizeTags()
{
    AgentManager::self()->d->synchronizeTags(*this);
}

void AgentInstance::synchronizeRelations()
{
    AgentManager::self()->d->synchronizeRelations(*this);
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
            qCWarning(AKONADICORE_LOG) << "Failed to place D-Bus call.";
        }
    } else {
        qCWarning(AKONADICORE_LOG) << "Unable to obtain agent interface";
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
            qCWarning(AKONADICORE_LOG) << "Failed to place D-Bus call.";
        }
    } else {
        qCWarning(AKONADICORE_LOG) << "Unable to obtain agent interface";
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
            qCWarning(AKONADICORE_LOG) << "Failed to place D-Bus call.";
        }
    } else {
        qCWarning(AKONADICORE_LOG) << "Unable to obtain control interface" << iface.lastError().message();
    }
}
