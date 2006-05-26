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

#ifndef AKONADISEARCHPROVIDERMANAGER_H
#define AKONADISEARCHPROVIDERMANAGER_H

#include "searchprovider.h"

namespace Akonadi {

class SearchProviderManager
{
  public:
    ~SearchProviderManager();

    static SearchProviderManager* self();

    /**
     * Returns a new search provider for the given mimetype which
     * uses the given connection for accessing the database.
     *
     * You have to delete the search provider yourself after use!
     */
    SearchProvider* createSearchProviderForMimetype( const QByteArray &mimeType, const AkonadiConnection *connection );

  private:
    SearchProviderManager();

    static SearchProviderManager* mSelf;
};

}

#endif
