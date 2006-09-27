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

#ifndef AKONADIPERSISTENTSEARCHMANAGER_H
#define AKONADIPERSISTENTSEARCHMANAGER_H

#include <QtCore/QMap>
#include <QtCore/QMutex>
#include <QtSql/QSqlDatabase>

#include "collection.h"
#include "persistentsearch.h"

namespace Akonadi {

class PersistentSearchManager
{
  public:
    ~PersistentSearchManager();

    static PersistentSearchManager* self();

    /**
     * Adds a new persistent search to the manager under the given name.
     *
     * The manager takes ownership of the persistent search.
     */
    void addPersistentSearch( const QString &identifier, PersistentSearch *search );

    /**
     * Removes the persistent search with the associated identifier from the
     * manager and deletes it.
     */
    void removePersistentSearch( const QString &identifier );

    /**
     * Returns a list of uids which match the persistent search with the given identifier.
     */
    QList<QByteArray> uids( const QString &identifier, const DataStore *store ) const;

    /**
     * Returns a list of objects which match the persistent search with the given identifier.
     */
    QList<QByteArray> objects( const QString &identifier, const DataStore *store ) const;

    /**
     * Returns a list of collections as representation of the persistent searches.
     */
    CollectionList collections() const;

  private:
    PersistentSearchManager();

    static PersistentSearchManager* mSelf;

    QMap<QString, PersistentSearch*> mList;
    mutable QMutex mListMutex;

    QSqlDatabase mDatabase;
};

}

#endif
