/*
    SPDX-FileCopyrightText: 2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

#include <QMetaType>
#include <QSharedDataPointer>

class QString;
class QWidget;

namespace Akonadi
{
class AgentType;
class AgentInstancePrivate;

/**
 * @short A representation of an agent instance.
 *
 * The agent instance is a representation of a running agent process.
 * It provides information about the instance and a reference to the
 * AgentType of that instance.
 *
 * All available agent instances can be retrieved from the AgentManager.
 *
 * @code
 *
 * Akonadi::AgentInstance::List instances = Akonadi::AgentManager::self()->instances();
 * for( const Akonadi::AgentInstance &instance : instances ) {
 *   qDebug() << "Name:" << instance.name() << "(" << instance.identifier() << ")";
 * }
 *
 * @endcode
 *
 * @note To find the collections belonging to an AgentInstance, use
 * CollectionFetchJob and supply AgentInstance::identifier() as the parameter
 * to CollectionFetchScope::setResource().
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADICORE_EXPORT AgentInstance
{
    friend class AgentManager;
    friend class AgentManagerPrivate;

public:
    /**
     * Describes a list of agent instances.
     */
    using List = QList<AgentInstance>;

    /**
     * Describes the status of the agent instance.
     */
    enum Status {
        Idle = 0, ///< The agent instance does currently nothing.
        Running, ///< The agent instance is working on something.
        Broken, ///< The agent instance encountered an error state.
        NotConfigured ///< The agent is lacking required configuration
    };

    /**
     * Creates a new agent instance object.
     */
    AgentInstance();

    /**
     * Creates an agent instance from an @p other agent instance.
     */
    AgentInstance(const AgentInstance &other);

    /**
     * Destroys the agent instance object.
     */
    ~AgentInstance();

    /**
     * Returns whether the agent instance object is valid.
     */
    [[nodiscard]] bool isValid() const;

    /**
     * Returns the agent type of this instance.
     */
    [[nodiscard]] AgentType type() const;

    /**
     * Returns the unique identifier of the agent instance.
     */
    [[nodiscard]] QString identifier() const;

    /**
     * Returns the user visible name of the agent instance.
     */
    [[nodiscard]] QString name() const;

    /**
     * Sets the user visible @p name of the agent instance.
     */
    void setName(const QString &name);

    /**
     * Returns the status of the agent instance.
     */
    [[nodiscard]] Status status() const;

    /**
     * Returns a textual presentation of the status of the agent instance.
     */
    [[nodiscard]] QString statusMessage() const;

    /**
     * Returns the progress of the agent instance in percent, or -1 if no
     * progress information are available.
     */
    [[nodiscard]] int progress() const;

    /**
     * Returns whether the agent instance is online currently.
     */
    [[nodiscard]] bool isOnline() const;

    /**
     * Sets @p online status of the agent instance.
     */
    void setIsOnline(bool online);

    /**
     * Triggers the agent instance to show its configuration dialog.
     *
     * @deprecated Use the new Akonadi::AgentConfigurationWidget and
     * Akonadi::AgentConfigurationDialog to display configuration dialogs
     * in-process
     *
     * @param parent Parent window for the configuration dialog.
     */
    AKONADICORE_DEPRECATED void configure(QWidget *parent = nullptr);

    /**
     * Triggers the agent instance to start synchronization.
     */
    void synchronize();

    /**
     * Triggers a synchronization of the collection tree by the given agent instance.
     */
    void synchronizeCollectionTree();

    /**
     * Triggers a synchronization of tags by the given agent instance.
     */
    void synchronizeTags();

    /**
     * Triggers a synchronization of relations by the given agent instance.
     */
    void synchronizeRelations();

    /**
     * @internal
     * @param other other agent instance
     */
    AgentInstance &operator=(const AgentInstance &other);

    /**
     * @internal
     * @param other other agent instance
     */
    [[nodiscard]] bool operator==(const AgentInstance &other) const;

    /**
     * Tell the agent to abort its current operation.
     * @since 4.4
     */
    void abortCurrentTask() const;

    /**
     * Tell the agent that its configuration has been changed remotely via D-Bus
     */
    void reconfigure() const;

    /**
     * Restart the agent process.
     */
    void restart() const;

private:
    /// @cond PRIVATE
    QSharedDataPointer<AgentInstancePrivate> d;
    /// @endcond
};

}

Q_DECLARE_TYPEINFO(Akonadi::AgentInstance, Q_RELOCATABLE_TYPE);

Q_DECLARE_METATYPE(Akonadi::AgentInstance)
