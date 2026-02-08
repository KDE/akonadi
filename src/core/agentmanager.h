/*
    SPDX-FileCopyrightText: 2006-2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

#include "agentinstance.h"
#include "agenttype.h"

#include <QObject>

#include <memory>

namespace Akonadi
{
class AgentManagerPrivate;
class Collection;

/*!
 * \brief Provides an interface to retrieve agent types and manage agent instances.
 *
 * This singleton class can be used to create or remove agent instances or trigger
 * synchronization of collections. Furthermore it provides information about status
 * changes of the agents.
 *
 * \code
 *
 *   Akonadi::AgentManager *manager = Akonadi::AgentManager::self();
 *
 *   Akonadi::AgentType::List types = manager->types();
 *   for ( const Akonadi::AgentType& type : types ) {
 *     qDebug() << "Type:" << type.name() << type.description();
 *   }
 *
 * \endcode
 *
 * \class Akonadi::AgentManager
 * \inheaderfile Akonadi/AgentManager
 * \inmodule AkonadiCore
 *
 * \author Tobias Koenig <tokoe@kde.org>
 */
class AKONADICORE_EXPORT AgentManager : public QObject
{
    friend class AgentInstance;
    friend class AgentInstanceCreateJobPrivate;
    friend class AgentManagerPrivate;

    Q_OBJECT

public:
    /*!
     * Returns the global instance of the agent manager.
     */
    static AgentManager *self();

    /*!
     * Destroys the agent manager.
     */
    ~AgentManager() override;

    /*!
     * Returns the list of all available agent types.
     */
    [[nodiscard]] AgentType::List types() const;

    /*!
     * Returns the agent type with the given \a identifier or
     * an invalid agent type if the identifier does not exist.
     */
    [[nodiscard]] AgentType type(const QString &identifier) const;

    /*!
     * Returns the list of all available agent instances.
     */
    [[nodiscard]] AgentInstance::List instances() const;

    /*!
     * Returns the agent instance with the given \a identifier or
     * an invalid agent instance if the identifier does not exist.
     *
     * Note that because a resource is a special case of an agent, the
     * identifier of a resource is the same as that of its agent instance.
     * \a identifier identifier to choose the agent instance
     */
    [[nodiscard]] AgentInstance instance(const QString &identifier) const;

    /*!
     * Removes the given agent \a instance.
     */
    void removeInstance(const AgentInstance &instance);

    /*!
     * Trigger a synchronization of the given collection by its owning resource agent.
     *
     * \a collection The collection to synchronize.
     */
    void synchronizeCollection(const Collection &collection);

    /*!
     * Trigger a synchronization of the given collection by its owning resource agent.
     *
     * \a collection The collection to synchronize.
     * \a recursive If true, the sub-collections are also synchronized
     *
     * \since 4.6
     */
    void synchronizeCollection(const Collection &collection, bool recursive);

Q_SIGNALS:
    /*!
     * This signal is emitted whenever a new agent type was installed on the system.
     *
     * \a type The new agent type.
     */
    void typeAdded(const Akonadi::AgentType &type);

    /*!
     * This signal is emitted whenever an agent type was removed from the system.
     *
     * \a type The removed agent type.
     */
    void typeRemoved(const Akonadi::AgentType &type);

    /*!
     * This signal is emitted whenever a new agent instance was created.
     *
     * \a instance The new agent instance.
     */
    void instanceAdded(const Akonadi::AgentInstance &instance);

    /*!
     * This signal is emitted whenever an agent instance was removed.
     *
     * \a instance The removed agent instance.
     */
    void instanceRemoved(const Akonadi::AgentInstance &instance);

    /*!
     * This signal is emitted whenever the status of an agent instance has
     * changed.
     *
     * \a instance The agent instance that status has changed.
     */
    void instanceStatusChanged(const Akonadi::AgentInstance &instance);

    /*!
     * This signal is emitted whenever the progress of an agent instance has
     * changed.
     *
     * \a instance The agent instance that progress has changed.
     */
    void instanceProgressChanged(const Akonadi::AgentInstance &instance);

    /*!
     * This signal is emitted whenever the name of the agent instance has changed.
     *
     * \a instance The agent instance that name has changed.
     */
    void instanceNameChanged(const Akonadi::AgentInstance &instance);

    /*!
     * This signal is emitted whenever the agent instance raised an error.
     *
     * \a instance The agent instance that raised the error.
     * \a message The i18n'ed error message.
     */
    void instanceError(const Akonadi::AgentInstance &instance, const QString &message);

    /*!
     * This signal is emitted whenever the agent instance raised a warning.
     *
     * \a instance The agent instance that raised the warning.
     * \a message The i18n'ed warning message.
     */
    void instanceWarning(const Akonadi::AgentInstance &instance, const QString &message);

    /*!
     * This signal is emitted whenever the online state of an agent changed.
     *
     * \a instance The agent instance that changed its online state.
     * \a online The new online state.
     * \since 4.2
     */
    void instanceOnline(const Akonadi::AgentInstance &instance, bool online);

private:
    explicit AgentManager();

    std::unique_ptr<AgentManagerPrivate> const d;
};

}
