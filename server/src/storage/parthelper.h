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
 * Helper methods that store data in a file instead of the database.
 *
 * @author Andras Mantia <amantia@kde.org>
 */
namespace PartHelper
{
  bool update( Part *part, const QByteArray &data, qint64 dataSize );
  bool insert( Part *part, qint64* insertId = 0 );
  /** Deletes @p part from the database and also removes existing filesystem data if needed. */
  bool remove( Part *part );
  /** Deletes all parts which match the given constraint, including all corresponding filesystem data. */
  bool remove( const QString &column, const QVariant &value );
  bool loadData( Part::List &parts );
  bool loadData( Part &part );
  QByteArray translateData( const QByteArray &data, bool isExternal  );
  QByteArray translateData( const Part& part );
  /** Returns the record with id @p id. */
  Part retrieveById( qint64 id );

  QString fileNameForId( qint64 id );
}

}

#endif
