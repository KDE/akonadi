/*
    SPDX-FileCopyrightText: 2006-2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentmanager.h"
#include "agentmanager_p.h"

#include "agentinstance_p.h"
#include "agenttype_p.h"
#include "collection.h"
#include "servermanager.h"
#include <QDBusConnection>

#include "shared/akranges.h"

#include <QDBusServiceWatcher>

using namespace Akonadi;
using namespace AkRanges;

// @cond PRIVATE

AgentInstance AgentManagerPrivate::createInstance(const AgentType &type)
{
    const QString &identifier = mManager->createAgentInstance(type.identifier());
    if (identifier.isEmpty()) {
        return AgentInstance();
    }

    return fillAgentInstanceLight(identifier);
}

void AgentManagerPrivate::agentTypeAdded(const QString &identifier)
{
    // Ignore agent types we already know about, for example because we called
    // readAgentTypes before.
    if (mTypes.contains(identifier)) {
        return;
    }

    if (mTypes.isEmpty()) {
        // The Akonadi ServerManager assumes that the server is up and running as soon
        // as it knows about at least one agent type.
        // If we Q_EMIT the typeAdded() signal here, it therefore thinks the server is
        // running. However, the AgentManager does not know about all agent types yet,
        // as the server might still have pending agentTypeAdded() signals, even though
        // it internally knows all agent types already.
        // This can cause situations where the client gets told by the ServerManager that
        // the server is running, yet the client will encounter an error because the
        // AgentManager doesn't know all types yet.
        //
        // Therefore, we read all agent types from the server here so they are known.
        readAgentTypes();
    }

    const AgentType type = fillAgentType(identifier);
    if (type.isValid()) {
        mTypes.insert(identifier, type);

        Q_EMIT mParent->typeAdded(type);
    }
}

void AgentManagerPrivate::agentTypeRemoved(const QString &typeName)
{
    auto it = mTypes.find(typeName);
    if (it == mTypes.cend()) {
        return;
    }

    const AgentType type = it.value();
    mTypes.erase(it);
    Q_EMIT mParent->typeRemoved(type);
}

void AgentManagerPrivate::agentInstanceAdded(const QString &identifier)
{
    const AgentInstance instance = fillAgentInstance(identifier);
    if (instance.isValid()) {
        // It is possible that this function is called when the instance is already
        // in our list we filled initially in the constructor.
        // This happens when the constructor is called during Akonadi startup, when
        // the agent processes are not fully loaded and have no D-Bus interface yet.
        // The server-side agent manager then emits the instance added signal when
        // the D-Bus interface for the agent comes up.
        // In this case, we simply notify that the instance status has changed.
        const bool newAgentInstance = !mInstances.contains(identifier);
        if (newAgentInstance) {
            mInstances.insert(identifier, instance);
            Q_EMIT mParent->instanceAdded(instance);
        } else {
            mInstances.remove(identifier);
            mInstances.insert(identifier, instance);
            Q_EMIT mParent->instanceStatusChanged(instance);
        }
    }
}

void AgentManagerPrivate::agentInstanceRemoved(const QString &identifier)
{
    auto it = mInstances.find(identifier);
    if (it == mInstances.cend()) {
        return;
    }

    const AgentInstance instance = it.value();
    mInstances.erase(it);
    Q_EMIT mParent->instanceRemoved(instance);
}

void AgentManagerPrivate::agentInstanceStatusChanged(const QString &identifier, int status, const QString &msg)
{
    auto it = mInstances.find(identifier);
    if (it == mInstances.cend()) {
        return;
    }

    AgentInstance &instance = it.value();
    instance.d->mStatus = status;
    instance.d->mStatusMessage = msg;

    Q_EMIT mParent->instanceStatusChanged(instance);
}

void AgentManagerPrivate::agentInstanceProgressChanged(const QString &identifier, uint progress, const QString &msg)
{
    auto it = mInstances.find(identifier);
    if (it == mInstances.cend()) {
        return;
    }

    AgentInstance &instance = it.value();
    instance.d->mProgress = progress;
    if (!msg.isEmpty()) {
        instance.d->mStatusMessage = msg;
    }

    Q_EMIT mParent->instanceProgressChanged(instance);
}

void AgentManagerPrivate::agentInstanceWarning(const QString &identifier, const QString &msg)
{
    auto it = mInstances.find(identifier);
    if (it == mInstances.cend()) {
        return;
    }

    Q_EMIT mParent->instanceWarning(it.value(), msg);
}

void AgentManagerPrivate::agentInstanceError(const QString &identifier, const QString &msg)
{
    auto it = mInstances.find(identifier);
    if (it == mInstances.cend()) {
        return;
    }

    Q_EMIT mParent->instanceError(it.value(), msg);
}

void AgentManagerPrivate::agentInstanceOnlineChanged(const QString &identifier, bool state)
{
    auto it = mInstances.find(identifier);
    if (it == mInstances.cend()) {
        return;
    }

    AgentInstance &instance = it.value();
    instance.d->mIsOnline = state;
    Q_EMIT mParent->instanceOnline(instance, state);
}

void AgentManagerPrivate::agentInstanceNameChanged(const QString &identifier, const QString &name)
{
    auto it = mInstances.find(identifier);
    if (it == mInstances.cend()) {
        return;
    }

    AgentInstance &instance = it.value();
    instance.d->mName = name;

    Q_EMIT mParent->instanceNameChanged(instance);
}

void AgentManagerPrivate::readAgentTypes()
{
    const QDBusReply<QStringList> types = mManager->agentTypes();
    if (types.isValid()) {
        const QStringList lst = types.value();
        for (const QString &type : lst) {
            const AgentType agentType = fillAgentType(type);
            if (agentType.isValid()) {
                mTypes.insert(type, agentType);
                Q_EMIT mParent->typeAdded(agentType);
            }
        }
    }
}

void AgentManagerPrivate::readAgentInstances()
{
    const QDBusReply<QStringList> instances = mManager->agentInstances();
    if (instances.isValid()) {
        const QStringList lst = instances.value();
        for (const QString &instance : lst) {
            const AgentInstance agentInstance = fillAgentInstance(instance);
            if (agentInstance.isValid()) {
                mInstances.insert(instance, agentInstance);
                Q_EMIT mParent->instanceAdded(agentInstance);
            }
        }
    }
}

AgentType AgentManagerPrivate::fillAgentType(const QString &identifier) const
{
    AgentType type;
    type.d->mIdentifier = identifier;
    type.d->mName = mManager->agentName(identifier);
    type.d->mDescription = mManager->agentComment(identifier);
    type.d->mIconName = mManager->agentIcon(identifier);
    type.d->mMimeTypes = mManager->agentMimeTypes(identifier);
    type.d->mCapabilities = mManager->agentCapabilities(identifier);
    type.d->mCustomProperties = mManager->agentCustomProperties(identifier);

    return type;
}

void AgentManagerPrivate::setName(const AgentInstance &instance, const QString &name)
{
    mManager->setAgentInstanceName(instance.identifier(), name);
}

void AgentManagerPrivate::setOnline(const AgentInstance &instance, bool state)
{
    mManager->setAgentInstanceOnline(instance.identifier(), state);
}

void AgentManagerPrivate::setActivities(const AgentInstance &instance, const QStringList &activities)
{
    mManager->setAgentInstanceActivities(instance.identifier(), activities);
}

void AgentManagerPrivate::setActivitiesEnabled(const AgentInstance &instance, bool enabled)
{
    mManager->setAgentInstanceActivitiesEnabled(instance.identifier(), enabled);
}

void AgentManagerPrivate::configure(const AgentInstance &instance, qlonglong winId)
{
    mManager->agentInstanceConfigure(instance.identifier(), winId);
}

void AgentManagerPrivate::synchronize(const AgentInstance &instance)
{
    mManager->agentInstanceSynchronize(instance.identifier());
}

void AgentManagerPrivate::synchronizeCollectionTree(const AgentInstance &instance)
{
    mManager->agentInstanceSynchronizeCollectionTree(instance.identifier());
}

void AgentManagerPrivate::synchronizeTags(const AgentInstance &instance)
{
    mManager->agentInstanceSynchronizeTags(instance.identifier());
}

AgentInstance AgentManagerPrivate::fillAgentInstance(const QString &identifier) const
{
    AgentInstance instance;

    const QString agentTypeIdentifier = mManager->agentInstanceType(identifier);
    auto typeIt = mTypes.find(agentTypeIdentifier);
    if (typeIt == mTypes.cend()) {
        return instance;
    }

    instance.d->mType = typeIt.value();
    instance.d->mIdentifier = identifier;
    instance.d->mName = mManager->agentInstanceName(identifier);
    instance.d->mStatus = mManager->agentInstanceStatus(identifier);
    instance.d->mStatusMessage = mManager->agentInstanceStatusMessage(identifier);
    instance.d->mProgress = mManager->agentInstanceProgress(identifier);
    instance.d->mIsOnline = mManager->agentInstanceOnline(identifier);
    // FIXME need to reactivate it
    // FIXME activities instance.d->mActivities = mManager->agentInstanceActivities(identifier);
    // FIXME activities instance.d->mActivitiesEnabled = mManager->agentInstanceActivitiesEnabled(identifier);

    return instance;
}

AgentInstance AgentManagerPrivate::fillAgentInstanceLight(const QString &identifier) const
{
    AgentInstance instance;

    const QString agentTypeIdentifier = mManager->agentInstanceType(identifier);
    Q_ASSERT_X(mTypes.contains(agentTypeIdentifier), "fillAgentInstanceLight", "Requests non-existing agent type");

    instance.d->mType = mTypes.value(agentTypeIdentifier);
    instance.d->mIdentifier = identifier;

    return instance;
}

void AgentManagerPrivate::createDBusInterface()
{
    mTypes.clear();
    mInstances.clear();

    using AgentManagerIface = org::freedesktop::Akonadi::AgentManager;
    mManager = std::make_unique<AgentManagerIface>(ServerManager::serviceName(ServerManager::Control),
                                                   QStringLiteral("/AgentManager"),
                                                   QDBusConnection::sessionBus(),
                                                   mParent);

    connect(mManager.get(), &AgentManagerIface::agentTypeAdded, this, &AgentManagerPrivate::agentTypeAdded);
    connect(mManager.get(), &AgentManagerIface::agentTypeRemoved, this, &AgentManagerPrivate::agentTypeRemoved);
    connect(mManager.get(), &AgentManagerIface::agentInstanceAdded, this, &AgentManagerPrivate::agentInstanceAdded);
    connect(mManager.get(), &AgentManagerIface::agentInstanceRemoved, this, &AgentManagerPrivate::agentInstanceRemoved);
    connect(mManager.get(), &AgentManagerIface::agentInstanceStatusChanged, this, &AgentManagerPrivate::agentInstanceStatusChanged);
    connect(mManager.get(), &AgentManagerIface::agentInstanceProgressChanged, this, &AgentManagerPrivate::agentInstanceProgressChanged);
    connect(mManager.get(), &AgentManagerIface::agentInstanceNameChanged, this, &AgentManagerPrivate::agentInstanceNameChanged);
    connect(mManager.get(), &AgentManagerIface::agentInstanceWarning, this, &AgentManagerPrivate::agentInstanceWarning);
    connect(mManager.get(), &AgentManagerIface::agentInstanceError, this, &AgentManagerPrivate::agentInstanceError);
    connect(mManager.get(), &AgentManagerIface::agentInstanceOnlineChanged, this, &AgentManagerPrivate::agentInstanceOnlineChanged);

    if (mManager->isValid()) {
        readAgentTypes();
        readAgentInstances();
    }
}

AgentManager *AgentManagerPrivate::mSelf = nullptr;

AgentManager::AgentManager()
    : QObject(nullptr)
    , d(new AgentManagerPrivate(this))
{
    // needed for queued connections on our signals
    qRegisterMetaType<Akonadi::AgentType>();
    qRegisterMetaType<Akonadi::AgentInstance>();

    d->createDBusInterface();

    d->mServiceWatcher = std::make_unique<QDBusServiceWatcher>(ServerManager::serviceName(ServerManager::Control),
                                                               QDBusConnection::sessionBus(),
                                                               QDBusServiceWatcher::WatchForRegistration);
    connect(d->mServiceWatcher.get(), &QDBusServiceWatcher::serviceRegistered, this, [this]() {
        if (d->mTypes.isEmpty()) { // just to be safe
            d->readAgentTypes();
        }
        if (d->mInstances.isEmpty()) {
            d->readAgentInstances();
        }
    });
}

/// @endcond

AgentManager::~AgentManager() = default;

AgentManager *AgentManager::self()
{
    if (!AgentManagerPrivate::mSelf) {
        AgentManagerPrivate::mSelf = new AgentManager();
    }

    return AgentManagerPrivate::mSelf;
}

AgentType::List AgentManager::types() const
{
    // Maybe the Control process is up and ready but we haven't been to the event loop yet so
    // QDBusServiceWatcher hasn't notified us yet.
    // In that case make sure to do it here, to avoid going into Broken state.
    if (d->mTypes.isEmpty()) {
        d->readAgentTypes();
    }
    return d->mTypes | Views::values | Actions::toQVector;
}

AgentType AgentManager::type(const QString &identifier) const
{
    return d->mTypes.value(identifier);
}

AgentInstance::List AgentManager::instances() const
{
    return d->mInstances | Views::values | Actions::toQVector;
}

AgentInstance AgentManager::instance(const QString &identifier) const
{
    return d->mInstances.value(identifier);
}

void AgentManager::removeInstance(const AgentInstance &instance)
{
    d->mManager->removeAgentInstance(instance.identifier());
}

void AgentManager::synchronizeCollection(const Collection &collection)
{
    synchronizeCollection(collection, false);
}

void AgentManager::synchronizeCollection(const Collection &collection, bool recursive)
{
    const QString resId = collection.resource();
    Q_ASSERT(!resId.isEmpty());
    d->mManager->agentInstanceSynchronizeCollection(resId, collection.id(), recursive);
}

#include "moc_agentmanager.cpp"

#include "moc_agentmanager_p.cpp"
