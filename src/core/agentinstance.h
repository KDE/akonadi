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

#ifndef AKONADI_AGENTINSTANCE_H
#define AKONADI_AGENTINSTANCE_H

#include "akonadicore_export.h"

#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>

class QString;
class QWidget;

namespace Akonadi {

class AgentType;

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
 * foreach ( const Akonadi::AgentInstance &instance, instances ) {
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
    typedef QList<AgentInstance> List;

    /**
     * Describes the status of the agent instance.
     */
    enum Status {
        Idle = 0, ///< The agent instance does currently nothing.
        Running,  ///< The agent instance is working on something.
        Broken,    ///< The agent instance encountered an error state.
        NotConfigured  ///< The agent is lacking required configuration
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
    bool isValid() const;

    /**
     * Returns the agent type of this instance.
     */
    AgentType type() const;

    /**
     * Returns the unique identifier of the agent instance.
     */
    QString identifier() const;

    /**
     * Returns the user visible name of the agent instance.
     */
    QString name() const;

    /**
     * Sets the user visible @p name of the agent instance.
     */
    void setName(const QString &name);

    /**
     * Returns the status of the agent instance.
     */
    Status status() const;

    /**
     * Returns a textual presentation of the status of the agent instance.
     */
    QString statusMessage() const;

    /**
     * Returns the progress of the agent instance in percent, or -1 if no
     * progress information are available.
     */
    int progress() const;

    /**
     * Returns whether the agent instance is online currently.
     */
    bool isOnline() const;

    /**
     * Sets @p online status of the agent instance.
     */
    void setIsOnline(bool online);

    /**
     * Triggers the agent instance to show its configuration dialog.
     *
     * @param parent Parent window for the configuration dialog.
     */
    void configure(QWidget *parent = Q_NULLPTR);

    /**
     * Triggers the agent instance to start synchronization.
     */
    void synchronize();

    /**
     * Triggers a synchronization of the collection tree by the given agent instance.
     */
    void synchronizeCollectionTree();

    /**
     * @internal
     * @param other other agent instance
     */
    AgentInstance &operator=(const AgentInstance &other);

    /**
     * @internal
     * @param other other agent instance
     */
    bool operator==(const AgentInstance &other) const;

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
    //@cond PRIVATE
    class Private;
    QSharedDataPointer<Private> d;
    //@endcond
};

}

Q_DECLARE_TYPEINFO(Akonadi::AgentInstance, Q_MOVABLE_TYPE);

Q_DECLARE_METATYPE(Akonadi::AgentInstance)

#endif
