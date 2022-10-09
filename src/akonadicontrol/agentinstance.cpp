/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentinstance.h"
#include "akonadicontrol_debug.h"

#include "agentmanager.h"

AgentInstance::AgentInstance(AgentManager &manager)
    : mManager(manager)
{
}

AgentInstance::~AgentInstance() = default;

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
    mAgentControlInterface = findInterface<org::freedesktop::Akonadi::Agent::Control>(Akonadi::DBus::Agent, "/");
    mAgentStatusInterface = findInterface<org::freedesktop::Akonadi::Agent::Status>(Akonadi::DBus::Agent, "/");

    if (mPendingQuit && mAgentControlInterface && mAgentControlInterface->isValid()) {
        mAgentControlInterface->quit();
        mPendingQuit = false;
    }

    if (!mAgentControlInterface || !mAgentStatusInterface) {
        return false;
    }

    mSearchInterface = findInterface<org::freedesktop::Akonadi::Agent::Search>(Akonadi::DBus::Agent, "/Search");

    connect(mAgentStatusInterface.get(),
            qOverload<int, const QString &>(&OrgFreedesktopAkonadiAgentStatusInterface::status),
            this,
            &AgentInstance::statusChanged);
    connect(mAgentStatusInterface.get(), &OrgFreedesktopAkonadiAgentStatusInterface::advancedStatus, this, &AgentInstance::advancedStatusChanged);
    connect(mAgentStatusInterface.get(), &OrgFreedesktopAkonadiAgentStatusInterface::percent, this, &AgentInstance::percentChanged);
    connect(mAgentStatusInterface.get(), &OrgFreedesktopAkonadiAgentStatusInterface::warning, this, &AgentInstance::warning);
    connect(mAgentStatusInterface.get(), &OrgFreedesktopAkonadiAgentStatusInterface::error, this, &AgentInstance::error);
    connect(mAgentStatusInterface.get(), &OrgFreedesktopAkonadiAgentStatusInterface::onlineChanged, this, &AgentInstance::onlineChanged);

    refreshAgentStatus();
    return true;
}

bool AgentInstance::obtainResourceInterface()
{
    mResourceInterface = findInterface<org::freedesktop::Akonadi::Resource>(Akonadi::DBus::Resource, "/");

    if (!mResourceInterface) {
        return false;
    }

    connect(mResourceInterface.get(), &OrgFreedesktopAkonadiResourceInterface::nameChanged, this, &AgentInstance::resourceNameChanged);
    refreshResourceStatus();
    return true;
}

bool AgentInstance::obtainPreprocessorInterface()
{
    mPreprocessorInterface = findInterface<org::freedesktop::Akonadi::Preprocessor>(Akonadi::DBus::Preprocessor, "/");
    return mPreprocessorInterface != nullptr;
}

void AgentInstance::statusChanged(int status, const QString &statusMsg)
{
    if (mStatus == status && mStatusMessage == statusMsg) {
        return;
    }
    mStatus = status;
    mStatusMessage = statusMsg;
    Q_EMIT mManager.agentInstanceStatusChanged(mIdentifier, mStatus, mStatusMessage);
}

void AgentInstance::advancedStatusChanged(const QVariantMap &status)
{
    Q_EMIT mManager.agentInstanceAdvancedStatusChanged(mIdentifier, status);
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
    Q_EMIT mManager.agentInstanceProgressChanged(mIdentifier, mPercent, QString());
}

void AgentInstance::warning(const QString &msg)
{
    Q_EMIT mManager.agentInstanceWarning(mIdentifier, msg);
}

void AgentInstance::error(const QString &msg)
{
    Q_EMIT mManager.agentInstanceError(mIdentifier, msg);
}

void AgentInstance::onlineChanged(bool state)
{
    if (mOnline == state) {
        return;
    }
    mOnline = state;
    Q_EMIT mManager.agentInstanceOnlineChanged(mIdentifier, state);
}

void AgentInstance::resourceNameChanged(const QString &name)
{
    if (name == mResourceName) {
        return;
    }
    mResourceName = name;
    Q_EMIT mManager.agentInstanceNameChanged(mIdentifier, name);
}

void AgentInstance::refreshAgentStatus()
{
    if (!hasAgentInterface()) {
        return;
    }

    // async calls so we are not blocked by misbehaving agents
    mAgentStatusInterface->callWithCallback(QStringLiteral("status"), QList<QVariant>(), this, SLOT(statusStateChanged(int)), SLOT(errorHandler(QDBusError)));
    mAgentStatusInterface->callWithCallback(QStringLiteral("statusMessage"),
                                            QList<QVariant>(),
                                            this,
                                            SLOT(statusMessageChanged(QString)),
                                            SLOT(errorHandler(QDBusError)));
    mAgentStatusInterface->callWithCallback(QStringLiteral("progress"), QList<QVariant>(), this, SLOT(percentChanged(int)), SLOT(errorHandler(QDBusError)));
    mAgentStatusInterface->callWithCallback(QStringLiteral("isOnline"), QList<QVariant>(), this, SLOT(onlineChanged(bool)), SLOT(errorHandler(QDBusError)));
}

void AgentInstance::refreshResourceStatus()
{
    if (!hasResourceInterface()) {
        return;
    }

    // async call so we are not blocked by misbehaving resources
    mResourceInterface->callWithCallback(QStringLiteral("name"), QList<QVariant>(), this, SLOT(resourceNameChanged(QString)), SLOT(errorHandler(QDBusError)));
}

void AgentInstance::errorHandler(const QDBusError &error)
{
    // avoid using the server tracer, can result in D-BUS lockups
    qCCritical(AKONADICONTROL_LOG) << QStringLiteral("D-Bus communication error '%1': '%2'").arg(error.name(), error.message());
    // TODO try again after some time, esp. on timeout errors
}

template<typename T>
std::unique_ptr<T> AgentInstance::findInterface(Akonadi::DBus::AgentType agentType, const char *path)
{
    auto iface = std::make_unique<T>(Akonadi::DBus::agentServiceName(mIdentifier, agentType), QLatin1String(path), QDBusConnection::sessionBus(), this);

    if (!iface || !iface->isValid()) {
        qCCritical(AKONADICONTROL_LOG) << Q_FUNC_INFO << "Cannot connect to agent instance with identifier" << mIdentifier
                                       << ", error message:" << (iface ? iface->lastError().message() : QString());
        return {};
    }
    return iface;
}
