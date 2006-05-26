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

#ifndef AKONADIPERSISTENTSEARCH_H
#define AKONADIPERSISTENTSEARCH_H

#include <QList>

#include "searchprovider.h"

namespace Akonadi {

class PersistentSearch
{
  public:
    /**
     * Creates a new persistant search with the given search criteria.
     *
     * @param searchCriteria The search criteria.
     * @param provider The search provider which queries the data from the
     *                 backend.
     *
     * The PersistantSearch object takes ownership of the provider.
     */
    PersistentSearch( const QList<QByteArray> &searchCriteria, SearchProvider *provider );

    /**
     * Destroys the persistant search.
     */
    ~PersistentSearch();

    /**
     * Returns a list of uids which match the stored search criteria.
     */
    QList<QByteArray> uids() const;

    /**
     * Returns a list of objects which match the stored search criteria.
     */
    QList<QByteArray> objects() const;

  private:
    QList<QByteArray> mQuery;
    SearchProvider *mProvider;
};

}

#endif
