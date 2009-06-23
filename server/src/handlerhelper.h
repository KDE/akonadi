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

#ifndef AKONADIHANDLERHELPER_H
#define AKONADIHANDLERHELPER_H

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

#include <entities.h>

namespace Akonadi {

/**
  Helper functions for command handlers.
*/
class HandlerHelper
{
  public:
    /**
      Removes leading and trailing delimiters.
    */
    static QByteArray normalizeCollectionName( const QByteArray &name );

    /**
      Returns the collection identified by the given id or path.
    */
    static Collection collectionFromIdOrName( const QByteArray &id );

    /**
      Returns the full path for the given collection.
    */
    static QString pathForCollection( const Collection &col );

    /**
      Returns the amount of existing items in the given collection.
      @return -1 on error
    */
    static int itemCount( const Collection &col );

    /**
      Returns the amount of existing items in the given collection
      which have a given flag set.
      @return -1 on error.
    */
    static int itemWithFlagCount( const Collection &col, const QString &flag );

    /**
      Returns the amount of existing items in the given collection
      which have a given flag not set.
      @return -1 on error
    */
    static int itemWithoutFlagCount( const Collection &col, const QString &flag );

    /**
      Returns the total size of all the items in the given collection.
      @return -1 on error
     */
    static qint64 itemsTotalSize( const Collection &col );

    /**
      Parse cache policy and update the given Collection object accoordingly.
      @param changed Indicates wether or not the cache policy already available in @p col
      has actually changed
      @todo Error handling.
    */
    static int parseCachePolicy( const QByteArray& data, Akonadi::Collection& col, int start = 0, bool* changed = 0 );

    /**
      Returns the protocol representation of the cache policy of the given
      Collection object.
    */
    static QByteArray cachePolicyToByteArray( const Collection &col );

    /**
      Returns the protocol representation of the given collection.
      Make sure DataStore::activeCachePolicy() has been called before to include
      the effective cache policy
    */
    static QByteArray collectionToByteArray( const Collection &col, bool hidden = false, bool includeStatistics = false );
};

}

#endif
