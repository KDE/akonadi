/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#ifndef AKONADIHANDLERHELPER_H
#define AKONADIHANDLERHELPER_H

#include "entities.h"

#include <QByteArray>
#include <QStack>
#include <QString>

namespace Akonadi
{
class Scope;
class ImapSet;

namespace Protocol
{
class Ancestor;
class CachePolicy;
class FetchCollectionsResponse;
class TagFetchScope;
class FetchTagsResponse;
using FetchTagsResponsePtr = QSharedPointer<FetchTagsResponse>;
class FetchRelationsResponse;
using FetchRelationsResponsePtr = QSharedPointer<FetchRelationsResponse>;
}

namespace Server
{
class CommandContext;
class Connection;
class AkonadiServer;

/**
  Helper functions for command handlers.
*/
class HandlerHelper
{
public:
    /**
      Returns the collection identified by the given id or path.
    */
    static Collection collectionFromIdOrName(const QByteArray &id);

    /**
      Returns the full path for the given collection.
    */
    static QString pathForCollection(const Collection &col);

    /**
      Returns the protocol representation of the cache policy of the given
      Collection object.
    */
    static Protocol::CachePolicy cachePolicyResponse(const Collection &col);

    /**
      Returns the protocol representation of the given collection.
      Make sure DataStore::activeCachePolicy() has been called before to include
      the effective cache policy
    */
    static Protocol::FetchCollectionsResponse fetchCollectionsResponse(AkonadiServer &akonadi, const Collection &col);

    /**
      Returns the protocol representation of the given collection.
      Make sure DataStore::activeCachePolicy() has been called before to include
      the effective cache policy
    */
    static Protocol::FetchCollectionsResponse
    fetchCollectionsResponse(AkonadiServer &akonadi,
                             const Collection &col,
                             const CollectionAttribute::List &attributeList,
                             bool includeStatistics = false,
                             int ancestorDepth = 0,
                             const QStack<Collection> &ancestors = QStack<Collection>(),
                             const QStack<CollectionAttribute::List> &ancestorAttributes = QStack<CollectionAttribute::List>(),
                             const QStringList &mimeTypes = QStringList());

    /**
      Returns the protocol representation of a collection ancestor chain.
    */
    static QVector<Protocol::Ancestor> ancestorsResponse(int ancestorDepth,
                                                         const QStack<Collection> &ancestors,
                                                         const QStack<CollectionAttribute::List> &_ancestorsAttributes = QStack<CollectionAttribute::List>());

    static Protocol::FetchTagsResponse fetchTagsResponse(const Tag &tag, const Protocol::TagFetchScope &tagFetchScope, Connection *connection = nullptr);

    static Protocol::FetchRelationsResponse fetchRelationsResponse(const Relation &relation);

    /**
      Converts a bytearray list of flag names into flag records.
      @throws HandlerException on errors during database operations
    */
    static Flag::List resolveFlags(const QSet<QByteArray> &flagNames);

    /**
      Converts a imap set of tags into tag records.
      @throws HandlerException on errors during database operations
    */
    static Tag::List resolveTagsByUID(const ImapSet &tags);

    static Tag::List resolveTagsByGID(const QStringList &tagsGIDs);

    static Tag::List resolveTagsByRID(const QStringList &tagsRIDs, const CommandContext &context);

    static Collection collectionFromScope(const Scope &scope, const CommandContext &context);

    static Tag::List tagsFromScope(const Scope &scope, const CommandContext &context);
};

} // namespace Server
} // namespace Akonadi

#endif
