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

#include "scope.h"
#include "fetchscope.h"
#include "libs/imapset_p.h"
#include "storage/countquerybuilder.h"
#include "storage/datastore.h"
#include "storage/itemretriever.h"

class FetchHelperTest;

namespace Akonadi {

class AkonadiConnection;
class ImapSet;
class Response;

class FetchHelper : public QObject
{
  Q_OBJECT

  public:
    FetchHelper( AkonadiConnection *connection, const Scope &scope );
    FetchHelper( const Scope &scope );

    void setStreamParser( ImapStreamParser *parser );

    int parseStream( const QByteArray &responseIdentifier );
    int parseStream( const QByteArray &responseIdentifier, const FetchScope &fetchScope );

    void setResolvePayloadPath( bool resolve );
    bool resolvePayloadPath() const;

  Q_SIGNALS:
    void responseAvailable( const Akonadi::Response& );

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
    QStack<Collection> ancestorsForItem( Collection::Id parentColId );
    static bool needsAccessTimeUpdate(const QVector< QByteArray >& parts);
    QVariant extractQueryResult( const QSqlQuery &query, ItemQueryColumns column ) const;

  private:
    ImapStreamParser *mStreamParser;

    AkonadiConnection *mConnection;
    Scope mScope;
    QHash<Collection::Id, QStack<Collection> > mAncestorCache;
    int mItemQueryColumnMap[ItemQueryColumnCount];
    FetchScope mFetchScope;
    bool mResolvePayloadPath;

    friend class ::FetchHelperTest;
};

}

#endif
