/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *   Copyright (c) 2007 Volker Krause <vkrause@kde.org>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef AGENTMANAGER_H
#define AGENTMANAGER_H

#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QStringList>

#include "agentinfo.h"
#include "tracerinterface.h"

class QDir;

namespace Akonadi {
  class ProcessControl;
}

/**
 * The agent manager has knowledge about all available agents (it scans
 * for .desktop files in the agent directory) and the available configured
 * instances.
 */
class AgentManager : public QObject
{
  Q_OBJECT
  Q_CLASSINFO( "D-Bus Interface", "org.kde.Akonadi.AgentManager" )

  public:
    /**
     * Creates a new agent manager.
     *
     * @param parent The parent object.
     */
    AgentManager( QObject *parent = 0 );

    /**
     * Destroys the agent manager.
     */
    ~AgentManager();

    /**
     * Called by the crash handler and dtor to terminate
     * the child processes.
     */
    void cleanup();

     /**
     * Returns the path of the config file.
     */
    static QString configPath( bool writeable );

    org::kde::Akonadi::Tracer* tracer() const { return mTracer; }

  public Q_SLOTS:
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
    QString agentIcon( const QString &identifier ) const;

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
    int agentInstanceStatus( const QString &identifier ) const;

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
     * @param windowId Parent window id for the configuration dialog.
     */
    void agentInstanceConfigure( const QString &identifier, qlonglong windowId );

    /**
     * Triggers the agent instance with the given @p identifier to start
     * synchronization.
     */
    void agentInstanceSynchronize( const QString &identifier );

    /**
      Trigger a synchronization of the collection tree by the given resource agent.
      @param identifier The resource agent identifier.
    */
    void agentInstanceSynchronizeCollectionTree( const QString &identifier );

    /**
      Trigger a synchronization of the given collection by its owning resource agent.
    */
    void agentInstanceSynchronizeCollection( const QString &identifier, qint64 collection );


    /**
      Returns if the agent instance @p identifier is in online mode.
    */
    bool agentInstanceOnline( const QString &identifier );

    /**
      Sets agent instance @p identifier to online or offline mode.
    */
    void setAgentInstanceOnline( const QString &identifier, bool state );

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
    void agentInstanceStatusChanged( const QString &agentIdentifier, int status, const QString &message );

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
     * @param name The new name of the agent instance.
     */
    void agentInstanceNameChanged( const QString &agentIdentifier, const QString &name );

  private Q_SLOTS:
    void updatePluginInfos();
    void serviceOwnerChanged( const QString&, const QString&, const QString& );
    void resourceStatusChanged( int status, const QString &message );
    void resourceProgressChanged( uint progress, const QString &message );
    void resourceNameChanged( const QString& );

  private:
    /**
     * Returns the list of directory paths where the .desktop files
     * for the plugins are located.
     */
    static QStringList pluginInfoPathList();

    /**
     * Loads the internal state from config file.
     */
    void load();

    /**
     * Saves internal state to the config file.
     */
    void save();

    /**
     * Reads the plugin infos from directory.
     */
    void readPluginInfos();

    /**
     * Reads the plugin infos from directory.
     *
     * @param directory the directory to get plugin information from
     */
    void readPluginInfos( const QDir &directory );

    bool checkAgentInterface( const QString &identifier ) const;
    bool checkInstance( const QString &identifier ) const;
    bool checkResourceInterface( const QString &identifier, const QString &method ) const;
    bool checkAgentExists( const QString &identifier ) const;
    void ensureAutoStart( const AgentInfo &info );
    void continueStartup();

  private:
    /**
     * The map which stores the .desktop file
     * entries for every agent type.
     *
     * Key is the agent type (e.g. 'file' or 'imap').
     */
    QHash<QString, AgentInfo> mAgents;

    /**
     * The map which stores the active instances.
     *
     * Key is the instance identifier.
     */
    QHash<QString, AgentInstanceInfo> mAgentInstances;

    org::kde::Akonadi::Tracer *mTracer;

    Akonadi::ProcessControl *mStorageController;
};

#endif
