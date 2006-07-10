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

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QtCore/QMap>
#include <QtCore/QStringList>

#include "tracerinterface.h"

class PluginManager : public QObject
{
  Q_OBJECT

  public:
    /**
     * Creates a new plugin manager.
     *
     * @param parent The parent object.
     */
    PluginManager( QObject *parent = 0 );

    /**
     * Destroys the plugin manager.
     */
    ~PluginManager();

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
     * Returns the name of the agent executable.
     */
    QString agentExecutable( const QString &identifier ) const;

    /**
     * Creates a new agent of the given agent type @p identifier.
     *
     * @return The identifier of the new agent if created successfully,
     *         an empty string otherwise.
     */
    QString createAgentInstance( const QString &identifier );

    /**
     * Removes the agent with the given @p identifier.
     */
    void removeAgentInstance( const QString &identifier );

  private Q_SLOTS:
    void updatePluginInfos();

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

    /**
     * The map which stores the .desktop file
     * entries for every plugin type.
     *
     * Key is the plugin type (e.g. 'file' or 'imap').
     */
    QMap<QString, PluginInfo> mPluginInfos;

    /**
     * The map which stores the instance specific
     * settings for every plugin type.
     */
    QMap<QString, InstanceInfo> mInstanceInfos;

    org::kde::Akonadi::Tracer *mTracer;
};

#endif
