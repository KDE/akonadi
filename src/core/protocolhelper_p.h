/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef AKONADI_PROTOCOLHELPER_P_H
#define AKONADI_PROTOCOLHELPER_P_H

#include "cachepolicy.h"
#include "collection.h"
#include "collectionutils.h"
#include "item.h"
#include "itemfetchscope.h"
#include "sharedvaluepool_p.h"
#include "attributeentity.h"
#include "tag.h"

#include <akonadi/private/imapparser_p.h>
#include <akonadi/private/protocol_p.h>

#include <boost/bind.hpp>
#include <algorithm>

namespace Akonadi {

struct ProtocolHelperValuePool
{
    typedef Internal::SharedValuePool<QByteArray, QVector> FlagPool;
    typedef Internal::SharedValuePool<QString, QVector> MimeTypePool;

    FlagPool flagPool;
    MimeTypePool mimeTypePool;
    QHash<Collection::Id, Collection> ancestorCollections;
};

/**
  @internal
  Helper methods for converting between libakonadi objects and their protocol
  representation.

  @todo Add unit tests for this.
  @todo Use exceptions for a useful error handling
*/
class ProtocolHelper
{
public:
    /** Part namespaces. */
    enum PartNamespace {
        PartGlobal,
        PartPayload,
        PartAttribute
    };

    /**
      Parse a cache policy definition.
      @param data The input data.
      @param policy The parsed cache policy.
      @param start Start of the data, ie. postion after the label.
      @returns Position in data after the cache policy description.
    */
    static int parseCachePolicy(const QByteArray &data, CachePolicy &policy, int start = 0);

    /**
      Convert a cache policy object into its protocol representation.
    */
    static QByteArray cachePolicyToByteArray(const CachePolicy &policy);

    /**
      Convert a ancestor chain from its protocol representation into an Entity object.
    */
    static void parseAncestors(const QByteArray &data, Entity *entity, int start = 0);

    /**
      Convert a ancestor chain from its protocol representation into an Entity object.

      This method allows to pass a @p valuePool which acts as cache, so ancestor paths for the
      same @p parentCollection don't have to be parsed twice.
    */
    static void parseAncestorsCached(const QByteArray &data, Entity *entity, Collection::Id parentCollection, ProtocolHelperValuePool *valuePool = 0, int start = 0);

    /**
      Parse a collection description.
      @param data The input data.
      @param collection The parsed collection.
      @param start Start of the data.
      @returns Position in data after the collection description.
    */
    static int parseCollection(const QByteArray &data, Collection &collection, int start = 0);

    /**
      Convert attributes to their protocol representation.
    */
    static QByteArray attributesToByteArray(const Entity &entity, bool ns = false);
    static QByteArray attributesToByteArray(const AttributeEntity &entity, bool ns = false);

    /**
      Encodes part label and namespace.
    */
    static QByteArray encodePartIdentifier(PartNamespace ns, const QByteArray &label, int version = 0);

    /**
      Decode part label and namespace.
    */
    static QByteArray decodePartIdentifier(const QByteArray &data, PartNamespace &ns);

    /**
      Converts the given set of items into a protocol representation.
      @throws A Akonadi::Exception if the item set contains items with missing/invalid identifiers.
    */
    template <typename T>
    static QByteArray entitySetToByteArray(const QList<T> &_objects, const QByteArray &command)
    {
        if (_objects.isEmpty()) {
            throw Exception("No objects specified");
        }

        typename T::List objects(_objects);

        QByteArray rv;
        std::sort(objects.begin(), objects.end(), boost::bind(&T::id, _1) < boost::bind(&T::id, _2));
        if (objects.first().isValid()) {
            // all items have a uid set
            rv += " " AKONADI_CMD_UID " ";
            if (!command.isEmpty()) {
                rv += command;
                rv += ' ';
            }
            QVector<typename T::Id>  uids;
            foreach (const T &object, objects) {
                uids << object.id();
            }
            ImapSet set;
            set.add(uids);
            rv += set.toImapSequenceSet();
            return rv;
        }

        // check if all items have a remote id
        if (std::find_if(objects.constBegin(), objects.constEnd(),
                         boost::bind(&QString::isEmpty, boost::bind(&T::remoteId, _1)))
            != objects.constEnd()) {
            throw Exception("No remote identifier specified");
        }

        // check if we have RIDs or HRIDs
        if (std::find_if(objects.constBegin(), objects.constEnd(),
                         !boost::bind(static_cast<bool (*)(const T &)>(&CollectionUtils::hasValidHierarchicalRID), _1))
            == objects.constEnd() && objects.size() == 1) {  // ### HRID sets are not yet specified
            // HRIDs
            rv += " " AKONADI_CMD_HRID " ";
            if (!command.isEmpty()) {
                rv += command;
                rv += ' ';
            }
            rv += '(' + hierarchicalRidToByteArray(objects.first()) + ')';
            return rv;
        }

        // RIDs
        QList<QByteArray> rids;
        foreach (const T &object, objects) {
            rids << ImapParser::quote(object.remoteId().toUtf8());
        }

        rv += " " AKONADI_CMD_RID " ";
        if (!command.isEmpty()) {
            rv += command;
            rv += ' ';
        }
        rv += '(';
        rv += ImapParser::join(rids, " ");
        rv += ')';
        return rv;
    }

    static QByteArray entitySetToByteArray(const QList<Akonadi::Item> &_objects, const QByteArray &command);

    static QByteArray tagSetToImapSequenceSet(const Akonadi::Tag::List &_objects);
    static QByteArray tagSetToByteArray(const Akonadi::Tag::List &_objects, const QByteArray &command);


    static QByteArray commandContextToByteArray(const Akonadi::Collection &collection, const Akonadi::Tag &tag,
                                                const Item::List &requestedItems, const QByteArray &command);

    /**
      Converts the given object identifier into a protocol representation.
      @throws A Akonadi::Exception if the item set contains items with missing/invalid identifiers.
    */
    template <typename T>
    static QByteArray entityIdToByteArray(const T &object, const QByteArray &command)
    {
        return entitySetToByteArray(typename T::List() << object, command);
    }

    /**
      Converts the given collection's hierarchical RID into a protocol representation.
      Assumes @p col has a valid hierarchical RID, so check that before!
    */
    static QByteArray hierarchicalRidToByteArray(const Collection &col);

    /**
      Converts the HRID of the given item into an ASAP protocol representation.
      Assumes @p item has a valid HRID.
    */
    static QByteArray hierarchicalRidToByteArray(const Item &item);

    /**
      Converts a given ItemFetchScope object into a protocol representation.
    */
    static QByteArray itemFetchScopeToByteArray(const ItemFetchScope &fetchScope);

    /**
      Parses a single line from an item fetch job result into an Item object.
     */
    static void parseItemFetchResult(const QList<QByteArray> &lineTokens, Item &item, ProtocolHelperValuePool *valuePool = 0);
    static void parseTagFetchResult(const QList<QByteArray> &lineTokens, Tag &tag);

    static QString akonadiStoragePath();
    static QString absolutePayloadFilePath(const QString &fileName);

    static bool streamPayloadToFile(const QByteArray &command, const QByteArray &data, QByteArray &error);

    static QByteArray listPreference(Collection::ListPurpose purpose, Collection::ListPreference preference);
    static QByteArray enabled(bool);
    static QByteArray referenced(bool);
};

}

#endif
