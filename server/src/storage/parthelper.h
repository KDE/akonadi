/***************************************************************************
 *   Copyright (C) 2009 by Andras Mantia <amantia@kde.org>                    *
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

#ifndef PARTHELPER_H
#define PARTHELPER_H

#include <QtGlobal>
#include "entities.h"

class QString;
class QVariant;

namespace Akonadi
{
/**
A specialized Part class that stores data in a file instead of the database.

	@author Andras Mantia <amantia@kde.org>
*/
class PartHelper
{
  public:
    PartHelper();
    ~PartHelper();

    static bool update( Part *part, const QByteArray &data, qint64 dataSize );
    static bool insert( Part *part, qint64* insertId = 0 );
    static bool remove( Part *part);
    static bool remove( const QString &column, const QVariant &value );
    static bool loadData( Part::List &parts );
    static bool loadData( Part &part );
    static QByteArray translateData( qint64 id, const QByteArray &data, bool isExternal  );
    static QByteArray translateData( const Part& part );
        /** Returns the record with id @p id. */
    static Part retrieveById( qint64 id );

    static QString fileNameForId( qint64 id );
};
}

#endif
