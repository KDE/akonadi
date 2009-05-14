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

#include "scope.h"
#include "libs/imapset_p.h"
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
  Q_OBJECT
  public:
    Fetch( Scope::SelectionScope scope );

    bool parseStream();

  private:
    void parseCommand( const QByteArray &line );
    void updateItemAccessTime();
    void triggerOnDemandFetch();
    void buildItemQuery();
    QueryBuilder buildPartQuery( const QStringList &partList, bool allPayload, bool allAttrs );
    void retrieveMissingPayloads( const QStringList &payloadList );
    void parseCommandStream();

  private:
    QueryBuilder mItemQuery;
    QList<QByteArray> mRequestedParts;
    Scope mScope;
    bool mCacheOnly;
    bool mFullPayload;
    bool mAllAttrs;
    bool mSizeRequested;
    bool mMTimeRequested;
    bool mExternalPayloadSupported;
};

}

#endif
