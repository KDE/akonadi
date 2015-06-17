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
#include <akonadi/private/scope_p.h>
#include <akonadi/private/tristate_p.h>

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
      @param policy The parsed cache policy.
      @returns Akonadi::CachePolicy
    */
    static CachePolicy parseCachePolicy(const Protocol::CachePolicy &policy);

    /**
      Convert a cache policy object into its protocol representation.
    */
    static Protocol::CachePolicy cachePolicyToProtocol(const CachePolicy &policy);

    /**
      Convert a ancestor chain from its protocol representation into an Entity object.
    */
    static void parseAncestors(const QVector<Protocol::Ancestor> &ancestors, Entity *entity);

    /**
      Convert a ancestor chain from its protocol representation into an Entity object.

      This method allows to pass a @p valuePool which acts as cache, so ancestor paths for the
      same @p parentCollection don't have to be parsed twice.
    */
    static void parseAncestorsCached(const QVector<Protocol::Ancestor> &ancestors,
                                     Entity *entity,
                                     Collection::Id parentCollection,
                                     ProtocolHelperValuePool *valuePool = 0);

    /**
      Parse a collection description.
      @param data The input data.
      @param requireParent Whether or not we require a parent as part of the data.
      @returns The parsed collection
    */
    static Collection parseCollection(const Protocol::FetchCollectionsResponse &data, bool requireParent = true);

    static void parseAttributes(const Protocol::Attributes &attributes, Entity *entity);
    static void parseAttributes(const Protocol::Attributes &attributes, AttributeEntity *entity);

    static CollectionStatistics parseCollectionStatistics(const Protocol::FetchCollectionStatsResponse &stats);

    /**
      Convert attributes to their protocol representation.
    */
    static Protocol::Attributes attributesToProtocol(const Entity &entity, bool ns = false);
    static Protocol::Attributes attributesToProtocol(const AttributeEntity &entity, bool ns = false);

    /**
      Encodes part label and namespace.
    */
    static QByteArray encodePartIdentifier(PartNamespace ns, const QByteArray &label);

    /**
      Decode part label and namespace.
    */
    static QByteArray decodePartIdentifier(const QByteArray &data, PartNamespace &ns);

    /**
      Converts the given set of items into a protocol representation.
      @throws A Akonadi::Exception if the item set contains items with missing/invalid identifiers.
    */
    template<typename T, template<typename> class Container>
    static Scope entitySetToScope(const Container<T> &_objects)
    {
        if (_objects.isEmpty()) {
            throw Exception("No objects specified");
        }

        Container<T> objects(_objects);

        QByteArray rv;
        std::sort(objects.begin(), objects.end(), boost::bind(&T::id, _1) < boost::bind(&T::id, _2));
        if (objects.first().isValid()) {
            QVector<typename T::Id>  uids;
            uids.reserve(objects.size());
            for (const T &object : objects) {
                uids << object.id();
            }
            ImapSet set;
            set.add(uids);
            return Scope(set);
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
            return hierarchicalRidToScope(objects.first());
        }

        // RIDs
        QStringList rids;
        rids.reserve(objects.size());
        for (const T &object : objects) {
            rids << object.remoteId();
        }

        return Scope(Scope::Rid, rids);
    }

    static Protocol::ScopeContext commandContextToProtocol(const Akonadi::Collection &collection, const Akonadi::Tag &tag,
                                                           const Item::List &requestedItems);

    /**
      Converts the given object identifier into a protocol representation.
      @throws A Akonadi::Exception if the item set contains items with missing/invalid identifiers.
    */
    template <typename T>
    static Scope entityToScope(const T &object)
    {
        return entitySetToScope(QVector<T>() << object);
    }

    /**
      Converts the given collection's hierarchical RID into a protocol representation.
      Assumes @p col has a valid hierarchical RID, so check that before!
    */
    static Scope hierarchicalRidToScope(const Collection &col);

    /**
      Converts the HRID of the given item into an ASAP protocol representation.
      Assumes @p item has a valid HRID.
    */
    static Scope hierarchicalRidToScope(const Item &item);

    /**
      Converts a given ItemFetchScope object into a protocol representation.
    */
    static Protocol::FetchScope itemFetchScopeToProtocol(const ItemFetchScope &fetchScope);

    /**
      Converts a given TagFetchScope object into a protocol representation.
    */
    static QVector<QByteArray> tagFetchScopeToProtocol(const TagFetchScope &fetchScope);

    /**
      Parses a single line from an item fetch job result into an Item object.
     */
    static Item parseItemFetchResult(const Protocol::FetchItemsResponse &data, ProtocolHelperValuePool *valuePool = 0);
    static Tag parseTagFetchResult(const Protocol::FetchTagsResponse &data);
    static Relation parseRelationFetchResult(const Protocol::FetchRelationsResponse &data);

    static QString akonadiStoragePath();
    static QString absolutePayloadFilePath(const QString &fileName);

    static bool streamPayloadToFile(const QString &file, const QByteArray &data, QByteArray &error);

    static Akonadi::Tristate listPreference(const Collection::ListPreference pref);
};

}

#endif
