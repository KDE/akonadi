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

#ifndef AKONADISEARCHPROVIDER_H
#define AKONADISEARCHPROVIDER_H

#include <QList>

#include "storage/datastore.h"

namespace Akonadi {

class SearchProvider
{
  public:
    SearchProvider();
    virtual ~SearchProvider();

    /**
     * Returns a list of supported mimetypes.
     */
    virtual QList<QByteArray> supportedMimeTypes() const = 0;

    /**
     * Starts a query for the given search criteria and
     * returns a list of matching uids.
     */
    virtual QList<QByteArray> queryForUids( const QList<QByteArray> &searchCriteria,
                                            const DataStore *store ) const = 0;

    /**
     * Starts a query for the given search criteria and
     * returns a list of matching objects.
     */
    virtual QList<QByteArray> queryForObjects( const QList<QByteArray> &searchCriteria,
                                               const DataStore *store ) const = 0;
};

}

#endif
