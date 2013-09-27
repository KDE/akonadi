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

    QVector<QByteArray> requestedParts() const { return d->requestedParts; }
    QStringList requestedPayloads() const { return d->requestedPayloads; }
    int ancestorDepth() const { return d->ancestorDepth; }
    bool cacheOnly() const { return d->cacheOnly; }
    bool checkCachedPayloadPartsOnly() const { return d->checkCachedPayloadPartsOnly; }
    bool fullPayload() const { return d->fullPayload; }
    bool allAttrs() const { return d->allAttrs; }
    bool sizeRequested() const { return d->sizeRequested; }
    bool mTimeRequested() const { return d->mTimeRequested; }
    bool externalPayloadSupported() const { return d->externalPayloadSupported; }
    bool remoteRevisionRequested() const { return d->remoteRevisionRequested; }
    bool ignoreErrors() const { return d->ignoreErrors; }
    bool flagsRequested() const { return d->flagsRequested; }
    bool remoteIdRequested() const { return d->remoteIdRequested; }
    bool gidRequested() const { return d->gidRequested; }
    QDateTime changedSince() const { return d->changedSince; }

  private:
    class Data: public QSharedData
    {
      public:
        Data();
        Data( const Data &other );

        void parseCommandStream();
        void parsePartList();

        QVector<QByteArray> requestedParts;
        QStringList requestedPayloads;
        int ancestorDepth;
        bool cacheOnly;
        bool checkCachedPayloadPartsOnly;
        bool fullPayload;
        bool allAttrs;
        bool sizeRequested;
        bool mTimeRequested;
        bool externalPayloadSupported;
        bool remoteRevisionRequested;
        bool ignoreErrors;
        bool flagsRequested;
        bool remoteIdRequested;
        bool gidRequested;
        QDateTime changedSince;

        ImapStreamParser *streamParser;
    };

    QSharedDataPointer<Data> const d;
};



class FetchHelper : public QObject
{
  Q_OBJECT

  public:
    FetchHelper( AkonadiConnection *connection, const Scope &scope );

    void setStreamParser( ImapStreamParser *parser );

    bool parseStream( const QByteArray &responseIdentifier );

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
    QVariant extractQueryResult(const QSqlQuery &query, ItemQueryColumns column) const;

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
