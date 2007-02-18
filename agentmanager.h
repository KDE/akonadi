/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

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

#include <QtCore/QObject>
#include <kdepim_export.h>

class QIcon;

namespace Akonadi {

/**
  This class provides an interface to manage agents.
*/
class AKONADI_EXPORT AgentManager : public QObject
{
  Q_OBJECT

  public:
    /**
     * This enum describes the possible states
     * of an agent.
     */
    enum Status
    {
      Ready = 0,
      Syncing,
      Error
    };

    /**
     * Creates a new agent manager.
     */
    AgentManager( QObject *parent = 0 );

    /**
     * Destroys the agent manager.
     */
    ~AgentManager();

    /**
     * Returns the list of identifiers of all available
     * agent types.
     */
    QStringList agentTypes() const;

    /**
     * Returns the i18n'ed name of the agent type for
     * the given @p identifier.
     */
    QString agentName( const QString &identifier ) const;

    /**
     * Returns the i18n'ed comment of the agent type for
     * the given @p identifier.
     */
    QString agentComment( const QString &identifier ) const;

    /**
     * Returns the icon name of the agent type for the
     * given @p identifier.
     */
    QString agentIconName( const QString &identifier ) const;

    /**
     * Returns the icon of the agent type for the
     * given @p identifier.
     */
    QIcon agentIcon( const QString &identifier ) const;

    /**
     * Returns a list of supported mimetypes of the agent type
     * for the given @p identifier.
     */
    QStringList agentMimeTypes( const QString &identifier ) const;

    /**
     * Returns a list of supported capabilities of the agent type
     * for the given @p identifier.
     */
    QStringList agentCapabilities( const QString &identifier ) const;

    /**
     * Creates a new agent of the given agent type @p identifier.
     *
     * @return The identifier of the new agent if created successfully,
     *         an empty string otherwise.
     *         The identifier consists of two parts, the type of the
     *         agent and an unique instance number, and looks like
     *         the following: 'file_1' or 'imap_267'.
     */
    QString createAgentInstance( const QString &identifier );

    /**
     * Removes the agent with the given @p identifier.
     */
    void removeAgentInstance( const QString &identifier );

    /**
     * Returns the type of the agent instance with the given @p identifier.
     */
    QString agentInstanceType( const QString &identifier );

    /**
     * Returns the list of identifiers of configured instances.
     */
    QStringList agentInstances() const;

    /**
     * Returns the current status code of the agent with the given @p identifier.
     */
    Status agentInstanceStatus( const QString &identifier ) const;

    /**
     * Returns the i18n'ed description of the current status of the agent with
     * the given @p identifier.
     */
    QString agentInstanceStatusMessage( const QString &identifier ) const;

    /**
     * Returns the current progress of the agent with the given @p identifier
     * in percentage.
     */
    uint agentInstanceProgress( const QString &identifier ) const;

    /**
     * Returns the i18n'ed description of the current progress of the agent with
     * the given @p identifier.
     */
    QString agentInstanceProgressMessage( const QString &identifier ) const;

    /**
     * Sets the @p name of the agent instance with the given @p identifier.
     */
    void setAgentInstanceName( const QString &identifier, const QString &name );

    /**
     * Returns the name of the agent instance with the given @p identifier.
     */
    QString agentInstanceName( const QString &identifier ) const;

    /**
     * Triggers the agent instance with the given @p identifier to show
     * its configuration dialog.
     */
    void agentInstanceConfigure( const QString &identifier );

    /**
     * Triggers the agent instance with the given @p identifier to start
     * synchronization.
     */
    void agentInstanceSynchronize( const QString &identifier );

  Q_SIGNALS:
    /**
     * This signal is emitted whenever a new agent type was installed on the system.
     *
     * @param agentType The identifier of the new agent type.
     */
    void agentTypeAdded( const QString &agentType );

    /**
     * This signal is emitted whenever an agent type was removed from the system.
     *
     * @param agentType The identifier of the removed agent type.
     */
    void agentTypeRemoved( const QString &agentType );

    /**
     * This signal is emitted whenever a new agent instance was created.
     *
     * @param agentIdentifier The identifier of the new agent instance.
     */
    void agentInstanceAdded( const QString &agentIdentifier );

    /**
     * This signal is emitted whenever an agent instance was removed.
     *
     * @param agentIdentifier The identifier of the removed agent instance.
     */
    void agentInstanceRemoved( const QString &agentIdentifier );

    /**
     * This signal is emitted whenever the status of an agent instance has
     * changed.
     *
     * @param agentIdentifier The identifier of the agent that has changed.
     * @param status The new status code.
     * @param message The i18n'ed description of the new status.
     */
    void agentInstanceStatusChanged( const QString &agentIdentifier, AgentManager::Status status, const QString &message );

    /**
     * This signal is emitted whenever the progress of an agent instance has
     * changed.
     *
     * @param agentIdentifier The identifier of the agent that has changed.
     * @param progress The new progress in percentage.
     * @param message The i18n'ed description of the new progress.
     */
    void agentInstanceProgressChanged( const QString &agentIdentifier, uint progress, const QString &message );

    /**
     * This signal is emitted whenever the name of the agent instance has changed.
     *
     * @param agentIdentifier The identifier of the agent that has changed.
     * @param name The new name of the agent identifier.
     */
    void agentInstanceNameChanged( const QString &agentIdentifier, const QString &name );

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void agentInstanceStatusChanged( const QString&, int, const QString& ) )
};

}

#endif
