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

#include <QtCore/QStringList>

#include "pluginmanager.h"
#include "profilemanager.h"
#include "resourceinterface.h"
#include "tracerinterface.h"

namespace Akonadi {
  class ProcessControl;
}

/**
 * Interface for handling profiles and agents in
 * the Akonadi server.
 */
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
     * Agent types specific methods
     */

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
     * Agent specific methods
     */

    /**
     * Creates a new agent of the given agent type @p type and configuration
     * identifier @p identifier.
     *
     * @return The identifier of the new agent if created successfully,
     *         an empty string otherwise.
     */
    QString createAgentInstance( const QString &type, const QString &identifier );

    /**
     * Removes the agent with the given @p identifier.
     */
    void removeAgentInstance( const QString &identifier );


    /**
     * Asks the resource agent to store the item with the given
     * identifier to the given @p collection as full or lightwight
     * version, depending on @p type.
     */
    bool requestItemDelivery( const QString &agentIdentifier, const QString &itemIdentifier,
                              const QString &collection, int type );

    /**
     * Profile specific methods
     */

    /**
     * Returns the list of identifiers of available profiles.
     */
    QStringList profiles() const;

    /**
     * Creates a new profile with the given @p identifier.
     *
     * @return true if created successfull, false a profile with the
     *         same @p identifier already exists.
     */
    bool createProfile( const QString &identifier );

    /**
     * Removes the profile with the given @p identifier.
     */
    void removeProfile( const QString &identifier );

    /**
     * Adds the agent with the given identifier to the profile with
     * the given identifier.
     *
     * @return true on success, false otherwise.
     */
    bool profileAddAgent( const QString &profileIdentifier, const QString &agentIdentifier );

    /**
     * Removes the agent with the given identifier from the profile with
     * the given identifier.
     *
     * @return true on success, false otherwise.
     */
    bool profileRemoveAgent( const QString &profileIdentifier, const QString &agentIdentifier );

    /**
     * Returns the list of identifiers of all agents in the profile
     * with the given identifier.
     */
    QStringList profileAgents( const QString &identifier ) const;

  private:
    class InstanceInfo {
      public:
        QString type;
        Akonadi::ProcessControl *controller;
        org::kde::Akonadi::Resource *interface;
    };

    PluginManager mPluginManager;
    ProfileManager mProfileManager;
    QMap<QString, InstanceInfo> mInstances;
    org::kde::Akonadi::Tracer *mTracer;
};

#endif
