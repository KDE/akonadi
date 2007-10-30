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
    static QString normalizeCollectionName( const QString &name );

    /**
      Returns the collection identified by the given id or path.
    */
    static Location collectionFromIdOrName( const QByteArray &id );

    /**
      Returns the full path for the given collection.
    */
    static QString pathForCollection( const Location &loc );

    /**
      Returns the amount of existing items in the given collection.
      @return -1 on error
    */
    static int itemCount( const Location &loc );

    /**
      Returns the amount of existing items in the given collection
      which have a given flag set.
      @return -1 on error.
    */
    static int itemWithFlagCount( const Location &loc, const QString &flag );

    /**
      Returns the amount of existing items in the given collection
      which have a given not flag set.
      @return -1 on error
    */
    static int itemWithoutFlagCount( const Location &loc, const QString &flag );
};

}

#endif
