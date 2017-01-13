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

#include "entities.h"

#include <QByteArray>
#include <QString>
#include <QStack>

namespace Akonadi
{

class Scope;
class ImapSet;

namespace Protocol
{
class Ancestor;
class CachePolicy;
class FetchCollectionsResponse;
class FetchTagsResponse;
class FetchRelationsResponse;
}

namespace Server
{

class CommandContext;
class Connection;

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
    static Protocol::FetchCollectionsResponse fetchCollectionsResponse(const Collection &col);

    /**
      Returns the protocol representation of the given collection.
      Make sure DataStore::activeCachePolicy() has been called before to include
      the effective cache policy
    */
    static Protocol::FetchCollectionsResponse fetchCollectionsResponse(const Collection &col,
            const CollectionAttribute::List &attributeList,
            bool includeStatistics = false,
            int ancestorDepth = 0,
            const QStack<Collection> &ancestors = QStack<Collection>(),
            const QStack<CollectionAttribute::List> &ancestorAttributes = QStack<CollectionAttribute::List>(),
            bool isReferenced = false,
            const QStringList &mimeTypes = QStringList());

    /**
      Returns the protocol representation of a collection ancestor chain.
    */
    static QVector<Protocol::Ancestor> ancestorsResponse(int ancestorDepth,
            const QStack<Collection> &ancestors,
            const QStack<CollectionAttribute::List> &_ancestorsAttributes = QStack<CollectionAttribute::List>());

    static Protocol::FetchTagsResponse fetchTagsResponse(const Tag &tag,
            bool withRID = false,
            Connection *connection = nullptr);

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

    static Tag::List resolveTagsByRID(const QStringList &tagsRIDs, CommandContext *context);

    static Collection collectionFromScope(const Scope &scope, Connection *connection);

    static Tag::List tagsFromScope(const Scope &scope, Connection *connection);
};

} // namespace Server
} // namespace Akonadi

#endif
