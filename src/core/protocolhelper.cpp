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

#include "protocolhelper_p.h"
#include "akonadicore_debug.h"
#include "attributefactory.h"
#include "collectionstatistics.h"
#include "item_p.h"
#include "collection_p.h"
#include "exception.h"
#include "itemserializer_p.h"
#include "itemserializerplugin.h"
#include "servermanager.h"
#include "tagfetchscope.h"
#include "persistentsearchattribute.h"

#include "private/protocol_p.h"
#include "private/xdgbasedirs_p.h"
#include "private/externalpartstorage_p.h"

#include <QDateTime>
#include <QFile>
#include <QVarLengthArray>
#include <QFileInfo>
#include <QDir>

#include <KLocalizedString>

using namespace Akonadi;

CachePolicy ProtocolHelper::parseCachePolicy(const Protocol::CachePolicy &policy)
{
    CachePolicy cp;
    cp.setCacheTimeout(policy.cacheTimeout());
    cp.setIntervalCheckTime(policy.checkInterval());
    cp.setInheritFromParent(policy.inherit());
    cp.setSyncOnDemand(policy.syncOnDemand());
    cp.setLocalParts(policy.localParts());
    return cp;
}

Protocol::CachePolicy ProtocolHelper::cachePolicyToProtocol(const CachePolicy &policy)
{
    Protocol::CachePolicy proto;
    proto.setCacheTimeout(policy.cacheTimeout());
    proto.setCheckInterval(policy.intervalCheckTime());
    proto.setInherit(policy.inheritFromParent());
    proto.setSyncOnDemand(policy.syncOnDemand());
    proto.setLocalParts(policy.localParts());
    return proto;
}


template<typename T>
inline static void parseAttributesImpl(const Protocol::Attributes &attributes, T *entity)
{
    for (auto iter = attributes.cbegin(), end = attributes.cend(); iter != end; ++iter) {
        Attribute *attribute = AttributeFactory::createAttribute(iter.key());
        if (!attribute) {
            qCWarning(AKONADICORE_LOG) << "Warning: unknown attribute" << iter.key();
            continue;
        }
        attribute->deserialize(iter.value());
        entity->addAttribute(attribute);
    }
}

template<typename T>
inline static void parseAncestorsCachedImpl(const QVector<Protocol::Ancestor> &ancestors, T *entity,
                                            Collection::Id parentCollection,
                                            ProtocolHelperValuePool *pool)
{
    if (!pool || parentCollection == -1) {
        // if no pool or parent collection id is provided we can't cache anything, so continue as usual
        ProtocolHelper::parseAncestors(ancestors, entity);
        return;
    }

    if (pool->ancestorCollections.contains(parentCollection)) {
        // ancestor chain is cached already, so use the cached value
        entity->setParentCollection(pool->ancestorCollections.value(parentCollection));
    } else {
        // not cached yet, parse the chain
        ProtocolHelper::parseAncestors(ancestors, entity);
        pool->ancestorCollections.insert(parentCollection, entity->parentCollection());
    }
}

template<typename T>
inline static Protocol::Attributes attributesToProtocolImpl(const T &entity, bool ns)
{
    Protocol::Attributes attributes;
    Q_FOREACH (const Attribute *attr, entity.attributes()) {
        attributes.insert(ProtocolHelper::encodePartIdentifier(ns ? ProtocolHelper::PartAttribute : ProtocolHelper::PartGlobal, attr->type()),
                          attr->serialized());
    }
    return attributes;
}

void ProtocolHelper::parseAncestorsCached(const QVector<Protocol::Ancestor> &ancestors,
                                          Item *item, Collection::Id parentCollection,
                                          ProtocolHelperValuePool *pool)
{
    parseAncestorsCachedImpl(ancestors, item, parentCollection, pool);
}

void ProtocolHelper::parseAncestorsCached(const QVector<Protocol::Ancestor> &ancestors,
                                          Collection *collection, Collection::Id parentCollection,
                                          ProtocolHelperValuePool *pool)
{
    parseAncestorsCachedImpl(ancestors, collection, parentCollection, pool);
}

void ProtocolHelper::parseAncestors(const QVector<Protocol::Ancestor> &ancestors, Item *item)
{
    Collection fakeCollection;
    parseAncestors(ancestors, &fakeCollection);

    item->setParentCollection(fakeCollection.parentCollection());
}

void ProtocolHelper::parseAncestors(const QVector<Protocol::Ancestor> &ancestors, Collection *collection)
{
    static const Collection::Id rootCollectionId = Collection::root().id();
    QList<QByteArray> parentIds;

    Collection *current = collection;
    for (const Protocol::Ancestor &ancestor : ancestors) {
        if (ancestor.id() == rootCollectionId) {
            current->setParentCollection(Collection::root());
            break;
        }

        Akonadi::Collection parentCollection(ancestor.id());
        parentCollection.setName(ancestor.name());
        parentCollection.setRemoteId(ancestor.remoteId());
        parseAttributesImpl(ancestor.attributes(), &parentCollection);
        current->setParentCollection(parentCollection);
        current = &current->parentCollection();
    }
}

static Collection::ListPreference parsePreference(Tristate value)
{
    switch (value) {
    case Tristate::True:
        return Collection::ListEnabled;
    case Tristate::False:
        return Collection::ListDisabled;
    case Tristate::Undefined:
        return Collection::ListDefault;
    }

    Q_ASSERT(false);
    return Collection::ListDefault;
}

CollectionStatistics ProtocolHelper::parseCollectionStatistics(const Protocol::FetchCollectionStatsResponse &stats)
{
    CollectionStatistics cs;
    cs.setCount(stats.count());
    cs.setSize(stats.size());
    cs.setUnreadCount(stats.unseen());
    return cs;
}

void ProtocolHelper::parseAttributes(const Protocol::Attributes &attributes, Item *item)
{
    parseAttributesImpl(attributes, item);
}

void ProtocolHelper::parseAttributes(const Protocol::Attributes &attributes, Collection *collection)
{
    parseAttributesImpl(attributes, collection);
}

void ProtocolHelper::parseAttributes(const Protocol::Attributes &attributes, Tag *tag)
{
    parseAttributesImpl(attributes, tag);
}

Protocol::Attributes ProtocolHelper::attributesToProtocol(const Item &item, bool ns)
{
    return attributesToProtocolImpl(item, ns);
}

Protocol::Attributes ProtocolHelper::attributesToProtocol(const Collection &collection, bool ns)
{
    return attributesToProtocolImpl(collection, ns);
}

Protocol::Attributes ProtocolHelper::attributesToProtocol(const Tag &tag, bool ns)
{
    return attributesToProtocolImpl(tag, ns);
}

Collection ProtocolHelper::parseCollection(const Protocol::FetchCollectionsResponse &data, bool requireParent)
{
    Collection collection(data.id());

    if (requireParent) {
        collection.setParentCollection(Collection(data.parentId()));
    }

    collection.setName(data.name());
    collection.setRemoteId(data.remoteId());
    collection.setRemoteRevision(data.remoteRevision());
    collection.setResource(data.resource());
    collection.setContentMimeTypes(data.mimeTypes());
    collection.setVirtual(data.isVirtual());
    collection.setStatistics(parseCollectionStatistics(data.statistics()));
    collection.setCachePolicy(parseCachePolicy(data.cachePolicy()));
    parseAncestors(data.ancestors(), &collection);
    collection.setEnabled(data.enabled());
    collection.setLocalListPreference(Collection::ListDisplay, parsePreference(data.displayPref()));
    collection.setLocalListPreference(Collection::ListIndex, parsePreference(data.indexPref()));
    collection.setLocalListPreference(Collection::ListSync, parsePreference(data.syncPref()));
    collection.setReferenced(data.referenced());

    if (!data.searchQuery().isEmpty()) {
        auto attr = collection.attribute<PersistentSearchAttribute>(Collection::AddIfMissing);
        attr->setQueryString(data.searchQuery());

        QVector<Collection> cols;
        cols.reserve(data.searchCollections().size());
        foreach (auto id, data.searchCollections()) {
            cols.push_back(Collection(id));
        }
        attr->setQueryCollections(cols);
    }

    parseAttributes(data.attributes(), &collection);

    collection.d_ptr->resetChangeLog();
    return collection;
}

QByteArray ProtocolHelper::encodePartIdentifier(PartNamespace ns, const QByteArray &label)
{
    switch (ns) {
    case PartGlobal:
        return label;
    case PartPayload:
        return "PLD:" + label;
    case PartAttribute:
        return "ATR:" + label;
    default:
        Q_ASSERT(false);
    }
    return QByteArray();
}

QByteArray ProtocolHelper::decodePartIdentifier(const QByteArray &data, PartNamespace &ns)
{
    if (data.startsWith("PLD:")) {     //krazy:exclude=strings
        ns = PartPayload;
        return data.mid(4);
    } else if (data.startsWith("ATR:")) {     //krazy:exclude=strings
        ns = PartAttribute;
        return data.mid(4);
    } else {
        ns = PartGlobal;
        return data;
    }
}

Protocol::ScopeContext ProtocolHelper::commandContextToProtocol(const Akonadi::Collection &collection,
        const Akonadi::Tag &tag,
        const Item::List &requestedItems)
{
    Protocol::ScopeContext ctx;
    if (tag.isValid()) {
        ctx.setContext(Protocol::ScopeContext::Tag, tag.id());
    }

    if (collection == Collection::root()) {
        if (requestedItems.isEmpty() && !tag.isValid()) {   // collection content listing
            throw Exception("Cannot perform item operations on root collection.");
        }
    } else {
        if (collection.isValid()) {
            ctx.setContext(Protocol::ScopeContext::Collection, collection.id());
        } else if (!collection.remoteId().isEmpty()) {
            ctx.setContext(Protocol::ScopeContext::Collection, collection.remoteId());
        }
    }

    return ctx;
}

Scope ProtocolHelper::hierarchicalRidToScope(const Collection &col)
{
    if (col == Collection::root()) {
        return Scope({ Scope::HRID(0) });
    }
    if (col.remoteId().isEmpty()) {
        return Scope();
    }

    QVector<Scope::HRID> chain;
    Collection c = col;
    while (!c.remoteId().isEmpty()) {
        chain.append(Scope::HRID(c.id(), c.remoteId()));
        c = c.parentCollection();
    }
    return Scope(chain + QVector<Scope::HRID> { Scope::HRID(0) });
}

Scope ProtocolHelper::hierarchicalRidToScope(const Item &item)
{
    return Scope(QVector<Scope::HRID>({ Scope::HRID(item.id(), item.remoteId()) }) + hierarchicalRidToScope(item.parentCollection()).hridChain());
}

Protocol::FetchScope ProtocolHelper::itemFetchScopeToProtocol(const ItemFetchScope &fetchScope)
{
    Protocol::FetchScope fs;
    QVector<QByteArray> parts;
    parts.reserve(fetchScope.payloadParts().size() + fetchScope.attributes().size());
    Q_FOREACH (const QByteArray &part, fetchScope.payloadParts()) {
        parts << ProtocolHelper::encodePartIdentifier(ProtocolHelper::PartPayload, part);
    }
    Q_FOREACH (const QByteArray &part, fetchScope.attributes()) {
        parts << ProtocolHelper::encodePartIdentifier(ProtocolHelper::PartAttribute, part);
    }
    fs.setRequestedParts(parts);

    // The default scope
    fs.setFetch(Protocol::FetchScope::Flags |
                Protocol::FetchScope::Size |
                Protocol::FetchScope::RemoteID |
                Protocol::FetchScope::RemoteRevision |
                Protocol::FetchScope::MTime);

    fs.setFetch(Protocol::FetchScope::FullPayload, fetchScope.fullPayload());
    fs.setFetch(Protocol::FetchScope::AllAttributes, fetchScope.allAttributes());
    fs.setFetch(Protocol::FetchScope::CacheOnly, fetchScope.cacheOnly());
    fs.setFetch(Protocol::FetchScope::CheckCachedPayloadPartsOnly, fetchScope.checkForCachedPayloadPartsOnly());
    fs.setFetch(Protocol::FetchScope::IgnoreErrors, fetchScope.ignoreRetrievalErrors());
    if (fetchScope.ancestorRetrieval() != ItemFetchScope::None) {
        switch (fetchScope.ancestorRetrieval()) {
        case ItemFetchScope::Parent:
            fs.setAncestorDepth(Protocol::Ancestor::ParentAncestor);
            break;
        case ItemFetchScope::All:
            fs.setAncestorDepth(Protocol::Ancestor::AllAncestors);
            break;
        default:
            Q_ASSERT(false);
        }
    } else {
        fs.setAncestorDepth(Protocol::Ancestor::NoAncestor);
    }

    if (fetchScope.fetchChangedSince().isValid()) {
        fs.setChangedSince(fetchScope.fetchChangedSince());
    }

    fs.setFetch(Protocol::FetchScope::RemoteID, fetchScope.fetchRemoteIdentification());
    fs.setFetch(Protocol::FetchScope::RemoteRevision, fetchScope.fetchRemoteIdentification());
    fs.setFetch(Protocol::FetchScope::GID, fetchScope.fetchGid());
    if (fetchScope.fetchTags()) {
        fs.setFetch(Protocol::FetchScope::Tags);
        if (!fetchScope.tagFetchScope().fetchIdOnly()) {
            if (fetchScope.tagFetchScope().attributes().isEmpty()) {
                fs.setTagFetchScope({ "ALL" });
            } else {
                fs.setTagFetchScope(fetchScope.tagFetchScope().attributes());
            }
        }
    }

    fs.setFetch(Protocol::FetchScope::VirtReferences, fetchScope.fetchVirtualReferences());
    fs.setFetch(Protocol::FetchScope::MTime, fetchScope.fetchModificationTime());
    fs.setFetch(Protocol::FetchScope::Relations, fetchScope.fetchRelations());

    return fs;
}

static Item::Flags convertFlags(const QVector<QByteArray> &flags, ProtocolHelperValuePool *valuePool)
{
#if __cplusplus >= 201103L || defined(__GNUC__) || defined(__clang__)
    // When the compiler supports thread-safe static initialization (mandated by the C++11 memory model)
    // then use it to share the common case of a single-item set only containing the \SEEN flag.
    // NOTE: GCC and clang has threadsafe static initialization for some time now, even without C++11.
    if (flags.size() == 1 && flags.first() == "\\SEEN") {
        static const Item::Flags sharedSeen = Item::Flags() << QByteArray("\\SEEN");
        return sharedSeen;
    }
#endif

    Item::Flags convertedFlags;
    convertedFlags.reserve(flags.size());
    for (const QByteArray &flag : flags) {
        if (valuePool) {
            convertedFlags.insert(valuePool->flagPool.sharedValue(flag));
        } else {
            convertedFlags.insert(flag);
        }
    }
    return convertedFlags;
}

Item ProtocolHelper::parseItemFetchResult(const Protocol::FetchItemsResponse &data, ProtocolHelperValuePool *valuePool)
{
    Item item;
    item.setId(data.id());
    item.setRevision(data.revision());
    item.setRemoteId(data.remoteId());
    item.setRemoteRevision(data.remoteRevision());
    item.setGid(data.gid());
    item.setStorageCollectionId(data.parentId());

    if (valuePool) {
        item.setMimeType(valuePool->mimeTypePool.sharedValue(data.mimeType()));
    } else {
        item.setMimeType(data.mimeType());
    }

    if (!item.isValid()) {
        return Item();
    }

    item.setFlags(convertFlags(data.flags(), valuePool));

    if (!data.tags().isEmpty()) {
        Tag::List tags;
        tags.reserve(data.tags().size());
        Q_FOREACH (const Protocol::FetchTagsResponse &tag, data.tags()) {
            tags.append(parseTagFetchResult(tag));
        }
        item.setTags(tags);
    }

    if (!data.relations().isEmpty()) {
        Relation::List relations;
        relations.reserve(data.relations().size());
        Q_FOREACH (const Protocol::FetchRelationsResponse &rel, data.relations()) {
            relations.append(parseRelationFetchResult(rel));
        }
        item.d_ptr->mRelations = relations;
    }

    if (!data.virtualReferences().isEmpty()) {
        Collection::List virtRefs;
        virtRefs.reserve(data.virtualReferences().size());
        Q_FOREACH (qint64 colId, data.virtualReferences()) {
            virtRefs.append(Collection(colId));
        }
        item.setVirtualReferences(virtRefs);
    }

    if (!data.cachedParts().isEmpty()) {
        QSet<QByteArray> cp;
        cp.reserve(data.cachedParts().size());
        Q_FOREACH (const QByteArray &ba, data.cachedParts()) {
            cp.insert(ba);
        }
        item.setCachedPayloadParts(cp);
    }

    item.setSize(data.size());
    item.setModificationTime(data.MTime());
    parseAncestorsCached(data.ancestors(), &item, data.parentId(), valuePool);

    Q_FOREACH (const Protocol::StreamPayloadResponse &part, data.parts()) {
        ProtocolHelper::PartNamespace ns;
        const QByteArray plainKey = decodePartIdentifier(part.payloadName(), ns);
        switch (ns) {
        case ProtocolHelper::PartPayload:
            ItemSerializer::deserialize(item, plainKey, part.data(), part.metaData().version(), part.metaData().isExternal());
            break;
        case ProtocolHelper::PartAttribute: {
            Attribute *attr = AttributeFactory::createAttribute(plainKey);
            Q_ASSERT(attr);
            if (part.metaData().isExternal()) {
                const QString filename = ExternalPartStorage::resolveAbsolutePath(part.data());
                QFile file(filename);
                if (file.open(QFile::ReadOnly)) {
                    attr->deserialize(file.readAll());
                } else {
                    qCWarning(AKONADICORE_LOG) << "Failed to open attribute file: " << filename;
                    delete attr;
                    attr = Q_NULLPTR;
                }
            } else {
                attr->deserialize(part.data());
            }
            if (attr) {
                item.addAttribute(attr);
            }
            break;
        }
        case ProtocolHelper::PartGlobal:
        default:
            qCWarning(AKONADICORE_LOG) << "Unknown item part type:" << part.payloadName();
        }
    }

    item.d_ptr->resetChangeLog();
    return item;
}

Tag ProtocolHelper::parseTagFetchResult(const Protocol::FetchTagsResponse &data)
{
    Tag tag;
    tag.setId(data.id());
    tag.setGid(data.gid());
    tag.setRemoteId(data.remoteId());
    tag.setType(data.type());
    tag.setParent(data.parentId() > 0 ? Tag(data.parentId()) : Tag());

    parseAttributes(data.attributes(), &tag);
    return tag;
}

Relation ProtocolHelper::parseRelationFetchResult(const Protocol::FetchRelationsResponse &data)
{
    Relation relation;
    relation.setLeft(Item(data.left()));
    relation.setRight(Item(data.right()));
    relation.setRemoteId(data.remoteId());
    relation.setType(data.type());
    return relation;
}

bool ProtocolHelper::streamPayloadToFile(const QString &fileName, const QByteArray &data, QByteArray &error)
{
    const QString filePath = ExternalPartStorage::resolveAbsolutePath(fileName);
    qCDebug(AKONADICORE_LOG) << filePath << fileName;
    if (!filePath.startsWith(ExternalPartStorage::akonadiStoragePath())) {
        qCWarning(AKONADICORE_LOG) << "Invalid file path" << fileName;
        error = "Invalid file path";
        return false;
    }
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(AKONADICORE_LOG) << "Failed to open destination payload file" << file.errorString();
        error = "Failed to store payload into file";
        return false;
    }
    if (file.write(data) != data.size()) {
        qCWarning(AKONADICORE_LOG) << "Failed to write all payload data to file";
        error = "Failed to store payload into file";
        return false;
    }
    qCDebug(AKONADICORE_LOG) << "Wrote" << data.size() << "bytes to " << file.fileName();

    // Make sure stuff is written to disk
    file.close();
    return true;
}

Akonadi::Tristate ProtocolHelper::listPreference(Collection::ListPreference pref)
{
    switch (pref) {
    case Collection::ListEnabled:
        return Tristate::True;
    case Collection::ListDisabled:
        return Tristate::False;
    case Collection::ListDefault:
        return Tristate::Undefined;
    }

    Q_ASSERT(false);
    return Tristate::Undefined;
}
