/*
    SPDX-FileCopyrightText: 2006-2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "agentmanagerinterface.h"

#include "agentinstance.h"
#include "agenttype.h"

#include <QHash>

#include <memory>

class QDBusServiceWatcher;

namespace Akonadi
{
class AgentManager;

/**
 * @internal
 */
class AgentManagerPrivate : public QObject
{
    friend class AgentManager;

    Q_OBJECT
public:
    explicit AgentManagerPrivate(AgentManager *parent)
        : mParent(parent)
    {
    }

    /*
     * Used by AgentInstanceCreateJob
     */
    AgentInstance createInstance(const AgentType &type);

    void agentTypeAdded(const QString &identifier);
    void agentTypeRemoved(const QString &identifier);
    void agentInstanceAdded(const QString &identifier);
    void agentInstanceRemoved(const QString &identifier);
    void agentInstanceStatusChanged(const QString &identifier, int status, const QString &msg);
    void agentInstanceProgressChanged(const QString &identifier, uint progress, const QString &msg);
    void agentInstanceNameChanged(const QString &identifier, const QString &name);
    void agentInstanceWarning(const QString &identifier, const QString &msg);
    void agentInstanceError(const QString &identifier, const QString &msg);
    void agentInstanceOnlineChanged(const QString &identifier, bool state);

    /**
     * Reads the information about all known agent types from the serverside
     * agent manager and updates mTypes, like agentTypeAdded() does.
     *
     * This will not remove agents from the internal map that are no longer on
     * the server.
     */
    void readAgentTypes();

    /**
     * Reads the information about all known agent instances from the server. If AgentManager
     * is created before the Akonadi.Control interface is registered, the agent
     * instances aren't immediately found then.
     */
    void readAgentInstances();

    void setName(const AgentInstance &instance, const QString &name);
    void setOnline(const AgentInstance &instance, bool state);
    void configure(const AgentInstance &instance, QWidget *parent);
    void synchronize(const AgentInstance &instance);
    void synchronizeCollectionTree(const AgentInstance &instance);
    void synchronizeTags(const AgentInstance &instance);

    void createDBusInterface();

    AgentType fillAgentType(const QString &identifier) const;
    AgentInstance fillAgentInstance(const QString &identifier) const;
    AgentInstance fillAgentInstanceLight(const QString &identifier) const;

    static AgentManager *mSelf;

    AgentManager *mParent = nullptr;
    std::unique_ptr<org::freedesktop::Akonadi::AgentManager> mManager;

    QHash<QString, AgentType> mTypes;
    QHash<QString, AgentInstance> mInstances;

    std::unique_ptr<QDBusServiceWatcher> mServiceWatcher;
};

}
