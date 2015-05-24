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
#include "imapset_p.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStack>

namespace Akonadi {

class Scope;

namespace Server {

class CommandContext;
class ImapStreamParser;
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
      Parse cache policy and update the given Collection object accoordingly.
      @param changed Indicates whether or not the cache policy already available in @p col
      has actually changed
      @todo Error handling.
    */
    static int parseCachePolicy(const QByteArray &data, Collection &col, int start = 0, bool *changed = 0);

    /**
      Returns the protocol representation of the cache policy of the given
      Collection object.
    */
    static QByteArray cachePolicyToByteArray(const Collection &col);

    static QByteArray tristateToByteArray(const Tristate &tristate);

    /**
      Returns the protocol representation of the given collection.
      Make sure DataStore::activeCachePolicy() has been called before to include
      the effective cache policy
    */
    static QByteArray collectionToByteArray(const Collection &col);

    /**
      Returns the protocol representation of the given collection.
      Make sure DataStore::activeCachePolicy() has been called before to include
      the effective cache policy
    */
    static QByteArray collectionToByteArray(const Collection &col, const CollectionAttribute::List &attributeList, bool includeStatistics = false,
                                            int ancestorDepth = 0, const QStack<Collection> &ancestors = QStack<Collection>(), const QStack<CollectionAttribute::List> &ancestorAttributes = QStack<CollectionAttribute::List>(), bool isReferenced = false, const QList<QByteArray> &mimeTypes = QList<QByteArray>());

    /**
      Returns the protocol representation of a collection ancestor chain.
    */
    static QByteArray ancestorsToByteArray(int ancestorDepth, const QStack<Collection> &ancestors, const QStack<CollectionAttribute::List> &_ancestorsAttributes = QStack<CollectionAttribute::List>());

    /**
      Parses the listing/ancestor depth parameter.
    */
    static int parseDepth(const QByteArray &depth);

    /**
      Converts a bytearray list of flag names into flag records.
      @throws HandlerException on errors during datbase operations
    */
    static Flag::List resolveFlags(const QVector<QByteArray> &flagNames);

    /**
      Converts a imap set of tags into tag records.
      @throws HandlerException on errors during datbase operations
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
