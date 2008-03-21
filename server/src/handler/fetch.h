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

#ifndef AKONADIFETCH_H
#define AKONADIFETCH_H

#include <handler.h>

#include <akonadi/private/imapset_p.h>

#include "storage/datastore.h"

namespace Akonadi {

class ImapSet;
class QueryBuilder;

/**
  @ingroup akonadi_server_handler

  Handler for the fetch command.
 */
class Fetch : public Handler
{
  public:
    Fetch();

    ~Fetch();

    bool handleLine(const QByteArray& line);

  private:
    bool parseCommand( const QByteArray &line );
    void updateItemAccessTime();
    void triggerOnDemandFetch();
    QueryBuilder buildPartQuery( const QStringList &partList );

  private:
    ImapSet mSet;
    bool mIsUidFetch;
    bool mAllParts;
    QList<QByteArray> mAttrList;

};

}

#endif
