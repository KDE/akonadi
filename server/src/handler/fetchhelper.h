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
#include "libs/imapset_p.h"
#include "storage/countquerybuilder.h"
#include "storage/datastore.h"
#include "storage/itemretriever.h"

class FetchHelperTest;

namespace Akonadi {

class AkonadiConnection;
class ImapSet;
class Response;

class FetchScope
{
  public:
    FetchScope();
    FetchScope( ImapStreamParser *parser );
    FetchScope( const FetchScope &other );
    ~FetchScope();

    QVector<QByteArray> requestedParts() const;
    void setRequestedParts( const QVector<QByteArray> &parts );
    QStringList requestedPayloads() const;
    void setRequestedPayloads( const QStringList &payloads );
    int ancestorDepth() const;
    void setAncestorDepth( int depth );
    bool cacheOnly() const;
    void setCacheOnly( bool cacheOnly );
    bool changedOnly() const;
    void setChangedOnly( bool changedOnly );
    bool checkCachedPayloadPartsOnly() const;
    void setCheckCachedPayloadPartsOnly( bool cachedOnly );
    bool fullPayload() const;
    void setFullPayload( bool fullPayload );
    bool allAttrs() const;
    void setAllAttrs( bool allAttrs );
    bool sizeRequested();
    void setSizeRequested( bool sizeRequested );
    bool mTimeRequested() const;
    void setMTimeRequested( bool mTimeRequested );
    bool externalPayloadSupported() const;
    void setExternalPayloadSupported( bool supported );
    bool remoteRevisionRequested() const;
    void setRemoteRevisionRequested( bool requested );
    bool ignoreErrors() const;
    void setIgnoreErrors( bool ignoreErrors );
    bool flagsRequested() const;
    void setFlagsRequested( bool flagsRequested );
    bool remoteIdRequested() const;
    void setRemoteIdRequested( bool requested );
    bool gidRequested() const;
    void setGidRequested( bool requested );
    QDateTime changedSince() const;
    void setChangedSince( const QDateTime &changedSince );

  private:
    class Data;
    QSharedDataPointer<Data> d;
};



class FetchHelper : public QObject
{
  Q_OBJECT

  public:
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

    FetchHelper( AkonadiConnection *connection, const Scope &scope );
    FetchHelper( const Scope &scope, const FetchScope &fetchScope );

    void setStreamParser( ImapStreamParser *parser );

    bool parseStream( const QByteArray &responseIdentifier );

    QSqlQuery buildItemQuery();

    QSqlQuery buildPartQuery( const QVector<QByteArray> &partList, bool allPayload, bool allAttrs );

    QSqlQuery buildFlagQuery();

    QVariant extractQueryResult( const QSqlQuery &query, ItemQueryColumns column ) const;

  Q_SIGNALS:
    void responseAvailable( const Akonadi::Response& );

  private:

    void updateItemAccessTime();
    void triggerOnDemandFetch();
    QStack<Collection> ancestorsForItem( Collection::Id parentColId );
    static bool needsAccessTimeUpdate(const QVector< QByteArray >& parts);

  private:
    ImapStreamParser *mStreamParser;

    AkonadiConnection *mConnection;
    Scope mScope;
    QHash<Collection::Id, QStack<Collection> > mAncestorCache;
    int mItemQueryColumnMap[ItemQueryColumnCount];
    FetchScope mFetchScope;

    friend class ::FetchHelperTest;
};

}

#endif
