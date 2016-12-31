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
#include "akonadicontrol_debug.h"

#include "agenttype.h"
#include "agentmanager.h"

#include <private/xdgbasedirs_p.h>

AgentInstance::AgentInstance(AgentManager *manager)
    : QObject(manager)
    , mManager(manager)
    , mAgentControlInterface(0)
    , mAgentStatusInterface(0)
    , mSearchInterface(0)
    , mResourceInterface(0)
    , mPreprocessorInterface(0)
    , mStatus(0)
    , mPercent(0)
    , mOnline(false)
    , mPendingQuit(false)
{
}

void AgentInstance::quit()
{
    if (mAgentControlInterface && mAgentControlInterface->isValid()) {
        mAgentControlInterface->quit();
    } else {
        mPendingQuit = true;
    }
}

void AgentInstance::cleanup()
{
    if (mAgentControlInterface && mAgentControlInterface->isValid()) {
        mAgentControlInterface->cleanup();
    }
}

bool AgentInstance::obtainAgentInterface()
{
    delete mAgentControlInterface;
    delete mAgentStatusInterface;

    mAgentControlInterface =
        findInterface<org::freedesktop::Akonadi::Agent::Control>(Akonadi::DBus::Agent, "/");
    mAgentStatusInterface =
        findInterface<org::freedesktop::Akonadi::Agent::Status>(Akonadi::DBus::Agent, "/");

    if (mPendingQuit && mAgentControlInterface && mAgentControlInterface->isValid()) {
        mAgentControlInterface->quit();
        mPendingQuit = false;
    }

    if (!mAgentControlInterface || !mAgentStatusInterface) {
        return false;
    }

    mSearchInterface =
        findInterface<org::freedesktop::Akonadi::Agent::Search>(Akonadi::DBus::Agent, "/Search");

    connect(mAgentStatusInterface, SIGNAL(status(int,QString)), SLOT(statusChanged(int,QString)));
    connect(mAgentStatusInterface, &OrgFreedesktopAkonadiAgentStatusInterface::advancedStatus, this, &AgentInstance::advancedStatusChanged);
    connect(mAgentStatusInterface, &OrgFreedesktopAkonadiAgentStatusInterface::percent, this, &AgentInstance::percentChanged);
    connect(mAgentStatusInterface, &OrgFreedesktopAkonadiAgentStatusInterface::warning, this, &AgentInstance::warning);
    connect(mAgentStatusInterface, &OrgFreedesktopAkonadiAgentStatusInterface::error, this, &AgentInstance::error);
    connect(mAgentStatusInterface, &OrgFreedesktopAkonadiAgentStatusInterface::onlineChanged, this, &AgentInstance::onlineChanged);

    refreshAgentStatus();
    return true;
}

bool AgentInstance::obtainResourceInterface()
{
    delete mResourceInterface;
    mResourceInterface =
        findInterface<org::freedesktop::Akonadi::Resource>(Akonadi::DBus::Resource, "/");

    if (!mResourceInterface) {
        return false;
    }

    connect(mResourceInterface, &OrgFreedesktopAkonadiResourceInterface::nameChanged, this, &AgentInstance::resourceNameChanged);
    refreshResourceStatus();
    return true;
}

bool AgentInstance::obtainPreprocessorInterface()
{
    delete mPreprocessorInterface;
    mPreprocessorInterface =
        findInterface<org::freedesktop::Akonadi::Preprocessor>(Akonadi::DBus::Preprocessor, "/");
    return mPreprocessorInterface;
}

void AgentInstance::statusChanged(int status, const QString &statusMsg)
{
    if (mStatus == status && mStatusMessage == statusMsg) {
        return;
    }
    mStatus = status;
    mStatusMessage = statusMsg;
    Q_EMIT mManager->agentInstanceStatusChanged(mIdentifier, mStatus, mStatusMessage);
}

void AgentInstance::advancedStatusChanged(const QVariantMap &status)
{
    Q_EMIT mManager->agentInstanceAdvancedStatusChanged(mIdentifier, status);
}

void AgentInstance::statusStateChanged(int status)
{
    statusChanged(status, mStatusMessage);
}

void AgentInstance::statusMessageChanged(const QString &msg)
{
    statusChanged(mStatus, msg);
}

void AgentInstance::percentChanged(int percent)
{
    if (mPercent == percent) {
        return;
    }
    mPercent = percent;
    Q_EMIT mManager->agentInstanceProgressChanged(mIdentifier, mPercent, QString());
}

void AgentInstance::warning(const QString &msg)
{
    Q_EMIT mManager->agentInstanceWarning(mIdentifier, msg);
}

void AgentInstance::error(const QString &msg)
{
    Q_EMIT mManager->agentInstanceError(mIdentifier, msg);
}

void AgentInstance::onlineChanged(bool state)
{
    if (mOnline == state) {
        return;
    }
    mOnline = state;
    Q_EMIT mManager->agentInstanceOnlineChanged(mIdentifier, state);
}

void AgentInstance::resourceNameChanged(const QString &name)
{
    if (name == mResourceName) {
        return;
    }
    mResourceName = name;
    Q_EMIT mManager->agentInstanceNameChanged(mIdentifier, name);
}

void AgentInstance::refreshAgentStatus()
{
    if (!hasAgentInterface()) {
        return;
    }

    // async calls so we are not blocked by misbehaving agents
    mAgentStatusInterface->callWithCallback(QStringLiteral("status"), QList<QVariant>(),
                                            this, SLOT(statusStateChanged(int)),
                                            SLOT(errorHandler(QDBusError)));
    mAgentStatusInterface->callWithCallback(QStringLiteral("statusMessage"), QList<QVariant>(),
                                            this, SLOT(statusMessageChanged(QString)),
                                            SLOT(errorHandler(QDBusError)));
    mAgentStatusInterface->callWithCallback(QStringLiteral("progress"), QList<QVariant>(),
                                            this, SLOT(percentChanged(int)),
                                            SLOT(errorHandler(QDBusError)));
    mAgentStatusInterface->callWithCallback(QStringLiteral("isOnline"), QList<QVariant>(),
                                            this, SLOT(onlineChanged(bool)),
                                            SLOT(errorHandler(QDBusError)));
}

void AgentInstance::refreshResourceStatus()
{
    if (!hasResourceInterface()) {
        return;
    }

    // async call so we are not blocked by misbehaving resources
    mResourceInterface->callWithCallback(QStringLiteral("name"), QList<QVariant>(),
                                         this, SLOT(resourceNameChanged(QString)),
                                         SLOT(errorHandler(QDBusError)));
}

void AgentInstance::errorHandler(const QDBusError &error)
{
    //avoid using the server tracer, can result in D-BUS lockups
    qCCritical(AKONADICONTROL_LOG) <<  QStringLiteral("D-Bus communication error '%1': '%2'").arg(error.name(), error.message()) ;
    // TODO try again after some time, esp. on timeout errors
}

template <typename T>
T *AgentInstance::findInterface(Akonadi::DBus::AgentType agentType, const char *path)
{
    T *iface = new T(Akonadi::DBus::agentServiceName(mIdentifier, agentType),
                     QLatin1String(path), QDBusConnection::sessionBus(), this);

    if (!iface || !iface->isValid()) {
        qCCritical(AKONADICONTROL_LOG) << Q_FUNC_INFO << "Cannot connect to agent instance with identifier"
                                       << mIdentifier << ", error message:"
                                       << (iface ? iface->lastError().message() : QString());
        delete iface;
        return Q_NULLPTR;
    }
    return iface;
}
