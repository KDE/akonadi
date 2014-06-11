/*
    Copyright (c) 2006-2008 Tobias Koenig <tokoe@kde.org>

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

#include "agentmanager.h"
#include "agentmanager_p.h"

#include "agenttype_p.h"
#include "agentinstance_p.h"
#include "dbusconnectionpool.h"
#include "servermanager.h"

#include "collection.h"

#include <QtDBus/QDBusServiceWatcher>
#include <QWidget>

#include <KLocale>
#include <KLocalizedString>

using namespace Akonadi;

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

    const AgentType type = fillAgentType(identifier);
    if (type.isValid()) {
        mTypes.insert(identifier, type);

        // The Akonadi ServerManager assumes that the server is up and running as soon
        // as it knows about at least one agent type.
        // If we emit the typeAdded() signal here, it therefore thinks the server is
        // running. However, the AgentManager does not know about all agent types yet,
        // as the server might still have pending agentTypeAdded() signals, even though
        // it internally knows all agent types already.
        // This can cause situations where the client gets told by the ServerManager that
        // the server is running, yet the client will encounter an error because the
        // AgentManager doesn't know all types yet.
        //
        // Therefore, we read all agent types from the server here so they are known.
        readAgentTypes();

        emit mParent->typeAdded(type);
    }
}

void AgentManagerPrivate::agentTypeRemoved(const QString &identifier)
{
    if (!mTypes.contains(identifier)) {
        return;
    }

    const AgentType type = mTypes.take(identifier);
    emit mParent->typeRemoved(type);
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
            emit mParent->instanceAdded(instance);
        } else {
            mInstances.remove(identifier);
            mInstances.insert(identifier, instance);
            emit mParent->instanceStatusChanged(instance);
        }
    }
}

void AgentManagerPrivate::agentInstanceRemoved(const QString &identifier)
{
    if (!mInstances.contains(identifier)) {
        return;
    }

    const AgentInstance instance = mInstances.take(identifier);
    emit mParent->instanceRemoved(instance);
}

void AgentManagerPrivate::agentInstanceStatusChanged(const QString &identifier, int status, const QString &msg)
{
    if (!mInstances.contains(identifier)) {
        return;
    }

    AgentInstance &instance = mInstances[identifier];
    instance.d->mStatus = status;
    instance.d->mStatusMessage = msg;

    emit mParent->instanceStatusChanged(instance);
}

void AgentManagerPrivate::agentInstanceProgressChanged(const QString &identifier, uint progress, const QString &msg)
{
    if (!mInstances.contains(identifier)) {
        return;
    }

    AgentInstance &instance = mInstances[identifier];
    instance.d->mProgress = progress;
    if (!msg.isEmpty()) {
        instance.d->mStatusMessage = msg;
    }

    emit mParent->instanceProgressChanged(instance);
}

void AgentManagerPrivate::agentInstanceWarning(const QString &identifier, const QString &msg)
{
    if (!mInstances.contains(identifier)) {
        return;
    }

    AgentInstance &instance = mInstances[identifier];
    emit mParent->instanceWarning(instance, msg);
}

void AgentManagerPrivate::agentInstanceError(const QString &identifier, const QString &msg)
{
    if (!mInstances.contains(identifier)) {
        return;
    }

    AgentInstance &instance = mInstances[identifier];
    emit mParent->instanceError(instance, msg);
}

void AgentManagerPrivate::agentInstanceOnlineChanged(const QString &identifier, bool state)
{
    if (!mInstances.contains(identifier)) {
        return;
    }

    AgentInstance &instance = mInstances[identifier];
    instance.d->mIsOnline = state;
    emit mParent->instanceOnline(instance, state);
}

void AgentManagerPrivate::agentInstanceNameChanged(const QString &identifier, const QString &name)
{
    if (!mInstances.contains(identifier)) {
        return;
    }

    AgentInstance &instance = mInstances[identifier];
    instance.d->mName = name;

    emit mParent->instanceNameChanged(instance);
}

void AgentManagerPrivate::readAgentTypes()
{
    const QDBusReply<QStringList> types = mManager->agentTypes();
    if (types.isValid()) {
        foreach (const QString &type, types.value()) {
            if (!mTypes.contains(type)) {
                agentTypeAdded(type);
            }
        }
    }
}

void AgentManagerPrivate::readAgentInstances()
{
    const QDBusReply<QStringList> instances = mManager->agentInstances();
    if (instances.isValid()) {
        foreach (const QString &instance, instances.value()) {
            if (!mInstances.contains(instance)) {
                agentInstanceAdded(instance);
            }
        }
    }
}

AgentType AgentManagerPrivate::fillAgentType(const QString &identifier) const
{
    AgentType type;
    type.d->mIdentifier = identifier;
    type.d->mName = mManager->agentName(identifier, KLocale::global()->language());
    type.d->mDescription = mManager->agentComment(identifier, KLocale::global()->language());
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

void AgentManagerPrivate::configure(const AgentInstance &instance, QWidget *parent)
{
    qlonglong winId = 0;
    if (parent) {
        winId = (qlonglong)(parent->window()->winId());
    }

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

AgentInstance AgentManagerPrivate::fillAgentInstance(const QString &identifier) const
{
    AgentInstance instance;

    const QString agentTypeIdentifier = mManager->agentInstanceType(identifier);
    if (!mTypes.contains(agentTypeIdentifier)) {
        return instance;
    }

    instance.d->mType = mTypes.value(agentTypeIdentifier);
    instance.d->mIdentifier = identifier;
    instance.d->mName = mManager->agentInstanceName(identifier);
    instance.d->mStatus = mManager->agentInstanceStatus(identifier);
    instance.d->mStatusMessage = mManager->agentInstanceStatusMessage(identifier);
    instance.d->mProgress = mManager->agentInstanceProgress(identifier);
    instance.d->mIsOnline = mManager->agentInstanceOnline(identifier);

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

void AgentManagerPrivate::serviceOwnerChanged(const QString &, const QString &oldOwner, const QString &)
{
    if (oldOwner.isEmpty()) {
        readAgentTypes();
        readAgentInstances();
    }
}

void AgentManagerPrivate::createDBusInterface()
{
    mTypes.clear();
    mInstances.clear();
    delete mManager;

    mManager = new org::freedesktop::Akonadi::AgentManager(ServerManager::serviceName(ServerManager::Control),
                                                           QStringLiteral("/AgentManager"),
                                                           DBusConnectionPool::threadConnection(), mParent);

    QObject::connect(mManager, SIGNAL(agentTypeAdded(QString)),
                     mParent, SLOT(agentTypeAdded(QString)));
    QObject::connect(mManager, SIGNAL(agentTypeRemoved(QString)),
                     mParent, SLOT(agentTypeRemoved(QString)));
    QObject::connect(mManager, SIGNAL(agentInstanceAdded(QString)),
                     mParent, SLOT(agentInstanceAdded(QString)));
    QObject::connect(mManager, SIGNAL(agentInstanceRemoved(QString)),
                     mParent, SLOT(agentInstanceRemoved(QString)));
    QObject::connect(mManager, SIGNAL(agentInstanceStatusChanged(QString,int,QString)),
                     mParent, SLOT(agentInstanceStatusChanged(QString,int,QString)));
    QObject::connect(mManager, SIGNAL(agentInstanceProgressChanged(QString,uint,QString)),
                     mParent, SLOT(agentInstanceProgressChanged(QString,uint,QString)));
    QObject::connect(mManager, SIGNAL(agentInstanceNameChanged(QString,QString)),
                     mParent, SLOT(agentInstanceNameChanged(QString,QString)));
    QObject::connect(mManager, SIGNAL(agentInstanceWarning(QString,QString)),
                     mParent, SLOT(agentInstanceWarning(QString,QString)));
    QObject::connect(mManager, SIGNAL(agentInstanceError(QString,QString)),
                     mParent, SLOT(agentInstanceError(QString,QString)));
    QObject::connect(mManager, SIGNAL(agentInstanceOnlineChanged(QString,bool)),
                     mParent, SLOT(agentInstanceOnlineChanged(QString,bool)));

    if (mManager->isValid()) {
        QDBusReply<QStringList> result = mManager->agentTypes();
        if (result.isValid()) {
            foreach (const QString &type, result.value()) {
                const AgentType agentType = fillAgentType(type);
                mTypes.insert(type, agentType);
            }
        }
        result = mManager->agentInstances();
        if (result.isValid()) {
            foreach (const QString &instance, result.value()) {
                const AgentInstance agentInstance = fillAgentInstance(instance);
                mInstances.insert(instance, agentInstance);
            }
        }
    } else {
        qWarning() << "AgentManager failed to get a valid AgentManager DBus interface. Error is:" << mManager->lastError().type() << mManager->lastError().name() << mManager->lastError().message();
    }
}

AgentManager *AgentManagerPrivate::mSelf = 0;

AgentManager::AgentManager()
    : QObject(0)
    , d(new AgentManagerPrivate(this))
{
    // needed for queued connections on our signals
    qRegisterMetaType<Akonadi::AgentType>();
    qRegisterMetaType<Akonadi::AgentInstance>();

    d->createDBusInterface();

    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(ServerManager::serviceName(ServerManager::Control),
                                                           DBusConnectionPool::threadConnection(),
                                                           QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            this, SLOT(serviceOwnerChanged(QString,QString,QString)));
}

// @endcond

AgentManager::~AgentManager()
{
    delete d;
}

AgentManager *AgentManager::self()
{
    if (!AgentManagerPrivate::mSelf) {
        AgentManagerPrivate::mSelf = new AgentManager();
    }

    return AgentManagerPrivate::mSelf;
}

AgentType::List AgentManager::types() const
{
    return d->mTypes.values();
}

AgentType AgentManager::type(const QString &identifier) const
{
    return d->mTypes.value(identifier);
}

AgentInstance::List AgentManager::instances() const
{
    return d->mInstances.values();
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
