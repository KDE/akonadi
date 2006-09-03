/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
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

#include <QtCore/QMap>
#include <QtCore/QStringList>

#include "resourceinterface.h"
#include "tracerinterface.h"

namespace Akonadi {
  class ProcessControl;
}

class AgentManager : public QObject
{
  Q_OBJECT

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
     * Sets the configuration of agent instance with the given @p identifier.
     *
     * @param data The configuration data in xml format.
     * @returns true if the configuration was accepted and applyed, false otherwise.
     */
    bool setAgentInstanceConfiguration( const QString &identifier, const QString &data );

    /**
     * Returns the current configuration data of the agent instance with the given
     * @p identifier or an empty string if no configuration is set.
     */
    QString agentInstanceConfiguration( const QString &identifier ) const;

    /**
     * Asks the agent to store the item with the given
     * identifier to the given @p collection as full or lightwight
     * version, depending on @p type.
     */
    bool requestItemDelivery( const QString &agentIdentifier, const QString &uid,
                              const QString &remoteId,
                              const QString &collection, int type );

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
     * This signal is emitted whenever the name of the agent instance has changed.
     *
     * @param agentIdentifier The identifier of the agent that has changed.
     * @param name The new name of the agent instance.
     */
    void agentInstanceNameChanged( const QString &agentIdentifier, const QString &name );

    /**
     * This signal is emitted whenever the configuration of the agent instance has changed.
     *
     * @param agentIdentifier The identifier of the agent that has changed.
     * @param data The new configuration data of the agent instance.
     */
    void agentInstanceConfigurationChanged( const QString &agentIdentifier, const QString &data );

  private Q_SLOTS:
    void updatePluginInfos();
    void resourceRegistered( const QString&, const QString&, const QString& );
    void resourceStatusChanged( int status, const QString &message );
    void resourceConfigurationChanged( const QString& );

  private:
    /**
     * Returns the directory path where the .desktop files
     * for the plugins are located.
     */
    static QString pluginInfoPath();

    /**
     * Returns the path of the config file.
     */
    static QString configPath();

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

    class PluginInfo
    {
      public:
        QString identifier;
        QString name;
        QString comment;
        QString icon;
        QStringList mimeTypes;
        QStringList capabilities;
        QString exec;
    };

    class InstanceInfo
    {
      public:
        uint instanceCounter;
    };

    class Instance
    {
      public:
        QString agentType;
        QString name;
        Akonadi::ProcessControl *controller;
        org::kde::Akonadi::Resource *interface;
    };

    /**
     * The map which stores the .desktop file
     * entries for every agent type.
     *
     * Key is the agent type (e.g. 'file' or 'imap').
     */
    QMap<QString, PluginInfo> mPluginInfos;

    /**
     * The map which stores the instance specific
     * settings for every plugin type.
     *
     * Key is the agent type.
     */
    QMap<QString, InstanceInfo> mInstanceInfos;

    /**
     * The map which stores the active instances.
     *
     * Key is the instance identifier.
     */
    QMap<QString, Instance> mInstances;

    org::kde::Akonadi::Tracer *mTracer;
};

#endif
