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

#ifndef PROFILEMANAGER_H
#define PROFILEMANAGER_H

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QStringList>

#include "tracerinterface.h"

class ProfileManager : public QObject
{
  Q_OBJECT

  public:
    /**
     * Creates a new profile manager.
     */
    ProfileManager( QObject *parent = 0 );

    /**
     * Destroys the profile manager.
     */
    ~ProfileManager();

    /**
     * Loads the profiles from the file.
     */
    void load();

    /**
     * Saves the profiles to the file.
     */
    void save();

  public Q_SLOTS:
    /**
     * Returns the list of identifiers of available profiles.
     */
    QStringList profiles() const;

    /**
     * Creates a new profile with the given @p identifier.
     *
     * @return true if created successful, false a profile with the
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

  Q_SIGNALS:
    /**
     * This signal is emitted whenever a new profile was created.
     *
     * @param profileIdentifier The identifier of the new profile.
     */
    void profileAdded( const QString &profileIdentifier );

    /**
     * This signal is emitted whenever a profile was removed.
     *
     * @param profileIdentifier The identifier of the removed profile.
     */
    void profileRemoved( const QString &profileIdentifier );

    /**
     * This signal is emitted whenever an agent was added to a profile.
     *
     * @param profileIdentifier The identifier of the profile.
     * @param agentIdentifier The identifier of the agent.
     */
    void profileAgentAdded( const QString &profileIdentifier, const QString &agentIdentifier );

    /**
     * This signal is emitted whenever an agent was removed from a profile.
     *
     * @param profileIdentifier The identifier of the profile.
     * @param agentIdentifier The identifier of the agent.
     */
    void profileAgentRemoved( const QString &profileIdentifier, const QString &agentIdentifier );

  private:
    QString profilePath() const;

    QMap<QString, QStringList> mProfiles;
    org::kde::Akonadi::Tracer *mTracer;
};

#endif
