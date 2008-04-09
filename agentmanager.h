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

#ifndef AKONADI_AGENTMANAGER_H
#define AKONADI_AGENTMANAGER_H

#include "akonadi_export.h"

#include <akonadi/agenttype.h>
#include <akonadi/agentinstance.h>

#include <QtCore/QObject>

namespace Akonadi {

class AgentManagerPrivate;
class Collection;

/**
 * This class provides an interface to manage agents.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADI_EXPORT AgentManager : public QObject
{
  friend class AgentInstance;
  friend class AgentInstanceCreateJob;
  friend class AgentManagerPrivate;

  Q_OBJECT

  public:
    /**
     * Returns the global instance of the agent manager.
     */
    static AgentManager *self();

    /**
     * Destroys the agent manager.
     */
    ~AgentManager();

    /**
     * Returns the list of all available agent types.
     */
    AgentType::List types() const;

    /**
     * Returns the agent type with the given @p identifier or
     * an invalid agent type if the identifier does not exist.
     */
    AgentType type( const QString &identififer ) const;

    /**
     * Returns the list of all configured agent instances.
     */
    AgentInstance::List instances() const;

    /**
     * Returns the agent instance with the given @p identifier or
     * an invalid agent instance if the identifier does not exist.
     */
    AgentInstance instance( const QString &identififer ) const;

    /**
     * Removes the given agent @p instance.
     */
    void removeInstance( const AgentInstance &instance );

    /**
     * Trigger a synchronization of the given collection by its owning resource agent.
     *
     * @param collection The collection to synchronize.
     */
    void synchronizeCollection( const Collection &collection );

  Q_SIGNALS:
    /**
     * This signal is emitted whenever a new agent type was installed on the system.
     *
     * @param type The new agent type.
     */
    void typeAdded( const Akonadi::AgentType &type );

    /**
     * This signal is emitted whenever an agent type was removed from the system.
     *
     * @param type The removed agent type.
     */
    void typeRemoved( const Akonadi::AgentType &type );

    /**
     * This signal is emitted whenever a new agent instance was created.
     *
     * @param instance The new agent instance.
     */
    void instanceAdded( const Akonadi::AgentInstance &instance );

    /**
     * This signal is emitted whenever an agent instance was removed.
     *
     * @param instance The removed agent instance.
     */
    void instanceRemoved( const Akonadi::AgentInstance &instance );

    /**
     * This signal is emitted whenever the status of an agent instance has
     * changed.
     *
     * @param instance The agent instance that status has changed.
     */
    void instanceStatusChanged( const Akonadi::AgentInstance &instance );

    /**
     * This signal is emitted whenever the progress of an agent instance has
     * changed.
     *
     * @param instance The agent instance that progress has changed.
     */
    void instanceProgressChanged( const Akonadi::AgentInstance &instance );

    /**
     * This signal is emitted whenever the name of the agent instance has changed.
     *
     * @param instance The agent instance that name has changed.
     */
    void instanceNameChanged( const Akonadi::AgentInstance &instance );

  private:
    AgentManager();

    AgentManagerPrivate* const d;

    Q_PRIVATE_SLOT( d, void agentTypeAdded( const QString& ) )
    Q_PRIVATE_SLOT( d, void agentTypeRemoved( const QString& ) )
    Q_PRIVATE_SLOT( d, void agentInstanceAdded( const QString& ) )
    Q_PRIVATE_SLOT( d, void agentInstanceRemoved( const QString& ) )
    Q_PRIVATE_SLOT( d, void agentInstanceStatusChanged( const QString&, int, const QString& ) )
    Q_PRIVATE_SLOT( d, void agentInstanceProgressChanged( const QString&, uint, const QString& ) )
    Q_PRIVATE_SLOT( d, void agentInstanceNameChanged( const QString&, const QString& ) )
};

}

#endif
