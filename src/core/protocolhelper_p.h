/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "cachepolicy.h"
#include "collection.h"
#include "collectionfetchscope.h"
#include "collectionutils.h"
#include "item.h"
#include "itemfetchscope.h"
#include "sharedvaluepool_p.h"
#include "tag.h"

#include "private/imapparser_p.h"
#include "private/protocol_p.h"
#include "private/scope_p.h"
#include "private/tristate_p.h"

#include <QString>

#include <algorithm>
#include <cassert>
#include <functional>
#include <set>
#include <type_traits>

namespace Akonadi
{
struct ProtocolHelperValuePool {
    using FlagPool = Internal::SharedValuePool<QByteArray, QList>;
    using MimeTypePool = Internal::SharedValuePool<QString, QList>;

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
        PartAttribute,
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
      Convert a ancestor chain from its protocol representation into an Item object.
    */
    static void parseAncestors(const QList<Protocol::Ancestor> &ancestors, Item *item);

    /**
      Convert a ancestor chain from its protocol representation into a Collection object.
    */
    static void parseAncestors(const QList<Protocol::Ancestor> &ancestors, Collection *collection);

    /**
      Convert a ancestor chain from its protocol representation into an Item object.

      This method allows to pass a @p valuePool which acts as cache, so ancestor paths for the
      same @p parentCollection don't have to be parsed twice.
    */
    static void
    parseAncestorsCached(const QList<Protocol::Ancestor> &ancestors, Item *item, Collection::Id parentCollection, ProtocolHelperValuePool *valuePool = nullptr);

    /**
      Convert a ancestor chain from its protocol representation into an Collection object.

      This method allows to pass a @p valuePool which acts as cache, so ancestor paths for the
      same @p parentCollection don't have to be parsed twice.
    */
    static void parseAncestorsCached(const QList<Protocol::Ancestor> &ancestors,
                                     Collection *collection,
                                     Collection::Id parentCollection,
                                     ProtocolHelperValuePool *valuePool = nullptr);
    /**
      Parse a collection description.
      @param data The input data.
      @param requireParent Whether or not we require a parent as part of the data.
      @returns The parsed collection
    */
    static Collection parseCollection(const Protocol::FetchCollectionsResponse &data, bool requireParent = true);

    static Tag parseTag(const Protocol::FetchTagsResponse &data);

    static void parseAttributes(const Protocol::Attributes &attributes, Item *item);
    static void parseAttributes(const Protocol::Attributes &attributes, Collection *collection);
    static void parseAttributes(const Protocol::Attributes &attributes, Tag *entity);

    static CollectionStatistics parseCollectionStatistics(const Protocol::FetchCollectionStatsResponse &stats);

    /**
      Convert attributes to their protocol representation.
    */
    static Protocol::Attributes attributesToProtocol(const Item &item, bool ns = false);
    static Protocol::Attributes attributesToProtocol(const Collection &collection, bool ns = false);
    static Protocol::Attributes attributesToProtocol(const Tag &entity, bool ns = false);
    static Protocol::Attributes attributesToProtocol(const std::vector<Attribute *> &modifiedAttributes, bool ns = false);

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
        using namespace std::placeholders;
        std::sort(objects.begin(), objects.end(), [](const T &a, const T &b) -> bool {
            return a.id() < b.id();
        });
        if (objects.at(0).isValid()) {
            QList<typename T::Id> uids;
            uids.reserve(objects.size());
            for (const T &object : objects) {
                uids << object.id();
            }
            ImapSet set;
            set.add(uids);
            return Scope(set);
        }

        if (entitySetHasGID(_objects)) {
            return entitySetToGID(_objects);
        }

        if (!entitySetHasRemoteIdentifier(_objects, std::mem_fn(&T::remoteId))) {
            throw Exception("No remote identifier specified");
        }

        // check if we have RIDs or HRIDs
        if (entitySetHasHRID(_objects)) {
            return hierarchicalRidToScope(objects.first());
        }

        return entitySetToRemoteIdentifier(Scope::Rid, _objects, std::mem_fn(&T::remoteId));
    }

    static Protocol::ScopeContext commandContextToProtocol(const Akonadi::Collection &collection, const Akonadi::Tag &tag, const Item::List &requestedItems);

    /**
      Converts the given object identifier into a protocol representation.
      @throws A Akonadi::Exception if the item set contains items with missing/invalid identifiers.
    */
    template<typename T>
    static Scope entityToScope(const T &object)
    {
        return entitySetToScope(QList<T>() << object);
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

    static Scope hierarchicalRidToScope(const Tag & /*tag*/)
    {
        assert(false);
        return Scope();
    }

    /**
      Converts a given ItemFetchScope object into a protocol representation.
    */
    static Protocol::ItemFetchScope itemFetchScopeToProtocol(const ItemFetchScope &fetchScope);
    static ItemFetchScope parseItemFetchScope(const Protocol::ItemFetchScope &fetchScope);

    static Protocol::CollectionFetchScope collectionFetchScopeToProtocol(const CollectionFetchScope &fetchScope);
    static CollectionFetchScope parseCollectionFetchScope(const Protocol::CollectionFetchScope &fetchScope);

    static Protocol::TagFetchScope tagFetchScopeToProtocol(const TagFetchScope &fetchScope);
    static TagFetchScope parseTagFetchScope(const Protocol::TagFetchScope &fetchScope);

    /**
     * Parses a single line from an item fetch job result into an Item object.
     * FIXME: std::optional
     */
    static Item
    parseItemFetchResult(const Protocol::FetchItemsResponse &data, const ItemFetchScope *fetchScope = nullptr, ProtocolHelperValuePool *valuePool = nullptr);
    static Tag parseTagFetchResult(const Protocol::FetchTagsResponse &data);
    static Relation parseRelationFetchResult(const Protocol::FetchRelationsResponse &data);

    static bool streamPayloadToFile(const QString &file, const QByteArray &data, QByteArray &error);

    static Akonadi::Tristate listPreference(const Collection::ListPreference pref);

private:
    template<typename T, template<typename> class Container>
    inline static bool entitySetHasGID(const Container<T> &objects)
    {
        if constexpr (std::is_same_v<T, Akonadi::Collection>) {
            Q_UNUSED(objects);
            return false;
        } else {
            return entitySetHasRemoteIdentifier(objects, std::mem_fn(&T::gid));
        }
    }

    template<typename T, template<typename> class Container>
    inline static Scope entitySetToGID(const Container<T> &objects)
    {
        if constexpr (std::is_same_v<T, Akonadi::Collection>) {
            Q_UNUSED(objects);
            return Scope();
        } else {
            return entitySetToRemoteIdentifier(Scope::Gid, objects, std::mem_fn(&T::gid));
        }
    }

    template<typename T, template<typename> class Container, typename RIDFunc>
    inline static bool entitySetHasRemoteIdentifier(const Container<T> &objects, const RIDFunc &ridFunc)
    {
        return std::find_if(objects.constBegin(),
                            objects.constEnd(),
                            [=](const T &obj) {
                                return ridFunc(obj).isEmpty();
                            })
            == objects.constEnd();
    }

    template<typename T, template<typename> class Container, typename RIDFunc>
    inline static Scope entitySetToRemoteIdentifier(Scope::SelectionScope scope, const Container<T> &objects, RIDFunc &&ridFunc)
    {
        QStringList rids;
        rids.reserve(objects.size());
        std::transform(objects.cbegin(), objects.cend(), std::back_inserter(rids), [=](const T &obj) -> QString {
            if constexpr (std::is_same_v<QString, std::remove_cvref_t<std::invoke_result_t<RIDFunc, const T &>>>) {
                return ridFunc(obj);
            } else {
                return QString::fromLatin1(ridFunc(obj));
            }
        });
        return Scope(scope, rids);
    }

    template<typename T, template<typename> class Container>
    inline static bool entitySetHasHRID(const Container<T> &objects)
    {
        if constexpr (std::is_same_v<T, Tag>) {
            return false;
        } else {
            return objects.size() == 1
                && std::find_if(objects.constBegin(),
                                objects.constEnd(),
                                [](const T &obj) -> bool {
                                    return !CollectionUtils::hasValidHierarchicalRID(obj);
                                })
                == objects.constEnd(); // ### HRID sets are not yet specified
        }
    }
};

}
