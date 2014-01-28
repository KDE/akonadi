/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#ifndef SEARCHMANAGER_H
#define SEARCHMANAGER_H

#include <QObject>
#include <QVector>
#include <QDBusConnection>

namespace Akonadi {

class NotificationCollector;
class AbstractSearchEngine;
class AbstractSearchPlugin;
class Collection;

/**
 * SearchManager creates and deletes persistent searches for all currently
 * active search engines.
 */
class SearchManager : public QObject
{
  Q_OBJECT
  Q_CLASSINFO( "D-Bus Interface", "org.freedesktop.Akonadi.SearchManager" )

  public:
    /** Create a new search manager with the given @p searchEngines. */
    explicit SearchManager( const QStringList &searchEngines, QObject *parent = 0 );
    ~SearchManager();

    /**
     * Returns a global instance of the search manager.
     */
    static SearchManager *instance();

    /**
     * This is called via D-Bus from AgentManager to register an agent with
     * search interface.
     */
    void registerInstance( const QString &id );

    /**
     * This is called via D-Bus from AgentManager to unregister an agent with
     * search interface.
     */
    void unregisterInstance( const QString &id );

    /**
     * Adds the given @p collection to the search.
     *
     * @returns true if the collection was added successfully, false otherwise.
     */
    bool addSearch( const Collection &collection );

    /**
     * Removes the collection with the given @p id from the search.
     *
     * @returns true if the collection was removed successfully, false otherwise.
     */
    bool removeSearch( qint64 id );

    /**
     * Returns currently available search plugins.
     */
    QVector<AbstractSearchPlugin *> searchPlugins() const;

  private Q_SLOTS:
    void addSearchInternal( const Collection &collection );
    void removeSearchInternal( qint64 id );

  private:
    void loadSearchPlugins();

    static SearchManager *sInstance;

    QVector<AbstractSearchEngine *> mEngines;
    QVector<AbstractSearchPlugin *> mPlugins;

    // agents dbus interface cache
    QDBusConnection mDBusConnection;
};

}

#endif
