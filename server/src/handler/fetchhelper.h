/***************************************************************************
 *   Copyright (C) 2006-2009 by Tobias Koenig <tokoe@kde.org>              *
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

#ifndef AKONADI_FETCHHELPER_H
#define AKONADI_FETCHHELPER_H

#include <QtCore/QStack>

#include "fetchscope.h"
#include "libs/imapset_p.h"
#include "storage/countquerybuilder.h"
#include "storage/datastore.h"
#include "storage/itemretriever.h"

class FetchHelperTest;

namespace Akonadi {

class ImapSet;

namespace Server {

class Connection;
class Response;

class FetchHelper : public QObject
{
  Q_OBJECT

  public:
    FetchHelper( Connection *connection, const Scope &scope, const FetchScope &fetchScope );

    bool fetchItems( const QByteArray &responseIdentifier );

  Q_SIGNALS:
    void responseAvailable( const Akonadi::Server::Response &response );

  private:
    enum ItemQueryColumns {
      ItemQueryPimItemIdColumn,
      ItemQueryPimItemRidColumn,
      ItemQueryMimeTypeColumn,
      ItemQueryRevColumn,
      ItemQueryRemoteRevisionColumn,
      ItemQuerySizeColumn,
      ItemQueryDatetimeColumn,
      ItemQueryCollectionIdColumn,
      ItemQueryPimItemGidColumn,
      ItemQueryColumnCount
    };

    void updateItemAccessTime();
    void triggerOnDemandFetch();
    QSqlQuery buildItemQuery();
    QSqlQuery buildPartQuery( const QVector<QByteArray> &partList, bool allPayload, bool allAttrs );
    QSqlQuery buildFlagQuery();
    QSqlQuery buildTagQuery();
    QSqlQuery buildVRefQuery();
    QStack<Collection> ancestorsForItem( Collection::Id parentColId );
    bool needsAccessTimeUpdate( const QVector<QByteArray> &parts ) const;
    QVariant extractQueryResult( const QSqlQuery &query, ItemQueryColumns column ) const;
    bool isScopeLocal( const Scope &scope );
    static QByteArray tagsToByteArray(const Tag::List &tags);

  private:
    ImapStreamParser *mStreamParser;

    Connection *mConnection;
    QHash<Collection::Id, QStack<Collection> > mAncestorCache;
    Scope mScope;
    FetchScope mFetchScope;
    int mItemQueryColumnMap[ItemQueryColumnCount];

    friend class ::FetchHelperTest;
};

} // namespace Server
} // namespace Akonadi

#endif
