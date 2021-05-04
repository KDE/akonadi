/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "akonadicore_debug.h"
#include "attributefactory.h"
#include "collection_p.h"
#include "collectionstatistics.h"
#include "exceptionbase.h"
#include "item_p.h"
#include "itemfetchscope.h"
#include "itemserializer_p.h"
#include "itemserializerplugin.h"
#include "persistentsearchattribute.h"
#include "protocolhelper_p.h"
#include "servermanager.h"
#include "tag_p.h"
#include "tagfetchscope.h"

#include "private/externalpartstorage_p.h"
#include "private/protocol_p.h"

#include <shared/akranges.h>

#include <QFile>
#include <QVarLengthArray>

using namespace Akonadi;
using namespace AkRanges;

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

template<typename T> inline static void parseAttributesImpl(const Protocol::Attributes &attributes, T *entity)
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
inline static void
parseAncestorsCachedImpl(const QVector<Protocol::Ancestor> &ancestors, T *entity, Collection::Id parentCollection, ProtocolHelperValuePool *pool)
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

template<typename T> inline static Protocol::Attributes attributesToProtocolImpl(const T &entity, bool ns)
{
    Protocol::Attributes attributes;
    const auto attrs = entity.attributes();
    for (const auto attr : attrs) {
        attributes.insert(ProtocolHelper::encodePartIdentifier(ns ? ProtocolHelper::PartAttribute : ProtocolHelper::PartGlobal, attr->type()),
                          attr->serialized());
    }
    return attributes;
}

void ProtocolHelper::parseAncestorsCached(const QVector<Protocol::Ancestor> &ancestors,
                                          Item *item,
                                          Collection::Id parentCollection,
                                          ProtocolHelperValuePool *pool)
{
    parseAncestorsCachedImpl(ancestors, item, parentCollection, pool);
}

void ProtocolHelper::parseAncestorsCached(const QVector<Protocol::Ancestor> &ancestors,
                                          Collection *collection,
                                          Collection::Id parentCollection,
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

Protocol::Attributes ProtocolHelper::attributesToProtocol(const std::vector<Attribute *> &modifiedAttributes, bool ns)
{
    Protocol::Attributes attributes;
    for (const Attribute *attr : modifiedAttributes) {
        attributes.insert(ProtocolHelper::encodePartIdentifier(ns ? ProtocolHelper::PartAttribute : ProtocolHelper::PartGlobal, attr->type()),
                          attr->serialized());
    }
    return attributes;
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

    if (!data.searchQuery().isEmpty()) {
        auto attr = collection.attribute<PersistentSearchAttribute>(Collection::AddIfMissing);
        attr->setQueryString(data.searchQuery());
        const auto cols = data.searchCollections() | Views::transform([](const auto id) {
                              return Collection{id};
                          })
            | Actions::toQVector;
        attr->setQueryCollections(cols);
    }

    parseAttributes(data.attributes(), &collection);

    collection.d_ptr->resetChangeLog();
    return collection;
}

Tag ProtocolHelper::parseTag(const Protocol::FetchTagsResponse &data)
{
    Tag tag(data.id());
    tag.setRemoteId(data.remoteId());
    tag.setGid(data.gid());
    tag.setType(data.type());
    tag.setParent(Tag(data.parentId()));
    parseAttributes(data.attributes(), &tag);
    tag.d_ptr->resetChangeLog();

    return tag;
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
    if (data.startsWith("PLD:")) { // krazy:exclude=strings
        ns = PartPayload;
        return data.mid(4);
    } else if (data.startsWith("ATR:")) { // krazy:exclude=strings
        ns = PartAttribute;
        return data.mid(4);
    } else {
        ns = PartGlobal;
        return data;
    }
}

Protocol::ScopeContext
ProtocolHelper::commandContextToProtocol(const Akonadi::Collection &collection, const Akonadi::Tag &tag, const Item::List &requestedItems)
{
    Protocol::ScopeContext ctx;
    if (tag.isValid()) {
        ctx.setContext(Protocol::ScopeContext::Tag, tag.id());
    }

    if (collection == Collection::root()) {
        if (requestedItems.isEmpty() && !tag.isValid()) { // collection content listing
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
        return Scope({Scope::HRID(0)});
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
    return Scope(chain + QVector<Scope::HRID>{Scope::HRID(0)});
}

Scope ProtocolHelper::hierarchicalRidToScope(const Item &item)
{
    return Scope(QVector<Scope::HRID>({Scope::HRID(item.id(), item.remoteId())}) + hierarchicalRidToScope(item.parentCollection()).hridChain());
}

Protocol::ItemFetchScope ProtocolHelper::itemFetchScopeToProtocol(const ItemFetchScope &fetchScope)
{
    Protocol::ItemFetchScope fs;
    QVector<QByteArray> parts;
    parts.reserve(fetchScope.payloadParts().size() + fetchScope.attributes().size());
    parts += fetchScope.payloadParts() | Views::transform(std::bind(encodePartIdentifier, PartPayload, std::placeholders::_1)) | Actions::toQVector;
    parts += fetchScope.attributes() | Views::transform(std::bind(encodePartIdentifier, PartAttribute, std::placeholders::_1)) | Actions::toQVector;
    fs.setRequestedParts(parts);

    // The default scope
    fs.setFetch(Protocol::ItemFetchScope::Flags | Protocol::ItemFetchScope::Size | Protocol::ItemFetchScope::RemoteID | Protocol::ItemFetchScope::RemoteRevision
                | Protocol::ItemFetchScope::MTime);

    fs.setFetch(Protocol::ItemFetchScope::FullPayload, fetchScope.fullPayload());
    fs.setFetch(Protocol::ItemFetchScope::AllAttributes, fetchScope.allAttributes());
    fs.setFetch(Protocol::ItemFetchScope::CacheOnly, fetchScope.cacheOnly());
    fs.setFetch(Protocol::ItemFetchScope::CheckCachedPayloadPartsOnly, fetchScope.checkForCachedPayloadPartsOnly());
    fs.setFetch(Protocol::ItemFetchScope::IgnoreErrors, fetchScope.ignoreRetrievalErrors());
    switch (fetchScope.ancestorRetrieval()) {
    case ItemFetchScope::Parent:
        fs.setAncestorDepth(Protocol::ItemFetchScope::ParentAncestor);
        break;
    case ItemFetchScope::All:
        fs.setAncestorDepth(Protocol::ItemFetchScope::AllAncestors);
        break;
    case ItemFetchScope::None:
        fs.setAncestorDepth(Protocol::ItemFetchScope::NoAncestor);
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    if (fetchScope.fetchChangedSince().isValid()) {
        fs.setChangedSince(fetchScope.fetchChangedSince());
    }

    fs.setFetch(Protocol::ItemFetchScope::RemoteID, fetchScope.fetchRemoteIdentification());
    fs.setFetch(Protocol::ItemFetchScope::RemoteRevision, fetchScope.fetchRemoteIdentification());
    fs.setFetch(Protocol::ItemFetchScope::GID, fetchScope.fetchGid());
    fs.setFetch(Protocol::ItemFetchScope::Tags, fetchScope.fetchTags());
    fs.setFetch(Protocol::ItemFetchScope::VirtReferences, fetchScope.fetchVirtualReferences());
    fs.setFetch(Protocol::ItemFetchScope::MTime, fetchScope.fetchModificationTime());
    fs.setFetch(Protocol::ItemFetchScope::Relations, fetchScope.fetchRelations());

    return fs;
}

ItemFetchScope ProtocolHelper::parseItemFetchScope(const Protocol::ItemFetchScope &fetchScope)
{
    ItemFetchScope ifs;
    const auto parts = fetchScope.requestedParts();
    for (const auto &part : parts) {
        if (part.startsWith("PLD:")) {
            ifs.fetchPayloadPart(part.mid(4), true);
        } else if (part.startsWith("ATR:")) {
            ifs.fetchAttribute(part.mid(4), true);
        }
    }

    if (fetchScope.fetch(Protocol::ItemFetchScope::FullPayload)) {
        ifs.fetchFullPayload(true);
    }
    if (fetchScope.fetch(Protocol::ItemFetchScope::AllAttributes)) {
        ifs.fetchAllAttributes(true);
    }
    if (fetchScope.fetch(Protocol::ItemFetchScope::CacheOnly)) {
        ifs.setCacheOnly(true);
    }
    if (fetchScope.fetch(Protocol::ItemFetchScope::CheckCachedPayloadPartsOnly)) {
        ifs.setCheckForCachedPayloadPartsOnly(true);
    }
    if (fetchScope.fetch(Protocol::ItemFetchScope::IgnoreErrors)) {
        ifs.setIgnoreRetrievalErrors(true);
    }
    switch (fetchScope.ancestorDepth()) {
    case Protocol::ItemFetchScope::ParentAncestor:
        ifs.setAncestorRetrieval(ItemFetchScope::Parent);
        break;
    case Protocol::ItemFetchScope::AllAncestors:
        ifs.setAncestorRetrieval(ItemFetchScope::All);
        break;
    default:
        ifs.setAncestorRetrieval(ItemFetchScope::None);
        break;
    }
    if (fetchScope.changedSince().isValid()) {
        ifs.setFetchChangedSince(fetchScope.changedSince());
    }
    if (fetchScope.fetch(Protocol::ItemFetchScope::RemoteID) || fetchScope.fetch(Protocol::ItemFetchScope::RemoteRevision)) {
        ifs.setFetchRemoteIdentification(true);
    }
    if (fetchScope.fetch(Protocol::ItemFetchScope::GID)) {
        ifs.setFetchGid(true);
    }
    if (fetchScope.fetch(Protocol::ItemFetchScope::Tags)) {
        ifs.setFetchTags(true);
    }
    if (fetchScope.fetch(Protocol::ItemFetchScope::VirtReferences)) {
        ifs.setFetchVirtualReferences(true);
    }
    if (fetchScope.fetch(Protocol::ItemFetchScope::MTime)) {
        ifs.setFetchModificationTime(true);
    }
    if (fetchScope.fetch(Protocol::ItemFetchScope::Relations)) {
        ifs.setFetchRelations(true);
    }

    return ifs;
}

Protocol::CollectionFetchScope ProtocolHelper::collectionFetchScopeToProtocol(const CollectionFetchScope &fetchScope)
{
    Protocol::CollectionFetchScope cfs;
    switch (fetchScope.listFilter()) {
    case CollectionFetchScope::NoFilter:
        cfs.setListFilter(Protocol::CollectionFetchScope::NoFilter);
        break;
    case CollectionFetchScope::Display:
        cfs.setListFilter(Protocol::CollectionFetchScope::Display);
        break;
    case CollectionFetchScope::Sync:
        cfs.setListFilter(Protocol::CollectionFetchScope::Sync);
        break;
    case CollectionFetchScope::Index:
        cfs.setListFilter(Protocol::CollectionFetchScope::Index);
        break;
    case CollectionFetchScope::Enabled:
        cfs.setListFilter(Protocol::CollectionFetchScope::Enabled);
        break;
    }
    cfs.setIncludeStatistics(fetchScope.includeStatistics());
    cfs.setResource(fetchScope.resource());
    cfs.setContentMimeTypes(fetchScope.contentMimeTypes());
    cfs.setAttributes(fetchScope.attributes());
    cfs.setFetchIdOnly(fetchScope.fetchIdOnly());
    switch (fetchScope.ancestorRetrieval()) {
    case CollectionFetchScope::None:
        cfs.setAncestorRetrieval(Protocol::CollectionFetchScope::None);
        break;
    case CollectionFetchScope::Parent:
        cfs.setAncestorRetrieval(Protocol::CollectionFetchScope::Parent);
        break;
    case CollectionFetchScope::All:
        cfs.setAncestorRetrieval(Protocol::CollectionFetchScope::All);
        break;
    }
    if (cfs.ancestorRetrieval() != Protocol::CollectionFetchScope::None) {
        cfs.setAncestorAttributes(fetchScope.ancestorFetchScope().attributes());
        cfs.setAncestorFetchIdOnly(fetchScope.ancestorFetchScope().fetchIdOnly());
    }
    cfs.setIgnoreRetrievalErrors(fetchScope.ignoreRetrievalErrors());

    return cfs;
}

CollectionFetchScope ProtocolHelper::parseCollectionFetchScope(const Protocol::CollectionFetchScope &fetchScope)
{
    CollectionFetchScope cfs;
    switch (fetchScope.listFilter()) {
    case Protocol::CollectionFetchScope::NoFilter:
        cfs.setListFilter(CollectionFetchScope::NoFilter);
        break;
    case Protocol::CollectionFetchScope::Display:
        cfs.setListFilter(CollectionFetchScope::Display);
        break;
    case Protocol::CollectionFetchScope::Sync:
        cfs.setListFilter(CollectionFetchScope::Sync);
        break;
    case Protocol::CollectionFetchScope::Index:
        cfs.setListFilter(CollectionFetchScope::Index);
        break;
    case Protocol::CollectionFetchScope::Enabled:
        cfs.setListFilter(CollectionFetchScope::Enabled);
        break;
    }
    cfs.setIncludeStatistics(fetchScope.includeStatistics());
    cfs.setResource(fetchScope.resource());
    cfs.setContentMimeTypes(fetchScope.contentMimeTypes());
    switch (fetchScope.ancestorRetrieval()) {
    case Protocol::CollectionFetchScope::None:
        cfs.setAncestorRetrieval(CollectionFetchScope::None);
        break;
    case Protocol::CollectionFetchScope::Parent:
        cfs.setAncestorRetrieval(CollectionFetchScope::Parent);
        break;
    case Protocol::CollectionFetchScope::All:
        cfs.setAncestorRetrieval(CollectionFetchScope::All);
        break;
    }
    if (cfs.ancestorRetrieval() != CollectionFetchScope::None) {
        cfs.ancestorFetchScope().setFetchIdOnly(fetchScope.ancestorFetchIdOnly());
        const auto attrs = fetchScope.ancestorAttributes();
        for (const auto &attr : attrs) {
            cfs.ancestorFetchScope().fetchAttribute(attr, true);
        }
    }
    const auto attrs = fetchScope.attributes();
    for (const auto &attr : attrs) {
        cfs.fetchAttribute(attr, true);
    }
    cfs.setFetchIdOnly(fetchScope.fetchIdOnly());
    cfs.setIgnoreRetrievalErrors(fetchScope.ignoreRetrievalErrors());

    return cfs;
}

Protocol::TagFetchScope ProtocolHelper::tagFetchScopeToProtocol(const TagFetchScope &fetchScope)
{
    Protocol::TagFetchScope tfs;
    tfs.setFetchIdOnly(fetchScope.fetchIdOnly());
    tfs.setAttributes(fetchScope.attributes());
    tfs.setFetchRemoteID(fetchScope.fetchRemoteId());
    tfs.setFetchAllAttributes(fetchScope.fetchAllAttributes());
    return tfs;
}

TagFetchScope ProtocolHelper::parseTagFetchScope(const Protocol::TagFetchScope &fetchScope)
{
    TagFetchScope tfs;
    tfs.setFetchIdOnly(fetchScope.fetchIdOnly());
    tfs.setFetchRemoteId(fetchScope.fetchRemoteID());
    tfs.setFetchAllAttributes(fetchScope.fetchAllAttributes());
    const auto attrs = fetchScope.attributes();
    for (const auto &attr : attrs) {
        tfs.fetchAttribute(attr, true);
    }
    return tfs;
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

Item ProtocolHelper::parseItemFetchResult(const Protocol::FetchItemsResponse &data,
                                          const Akonadi::ItemFetchScope *fetchScope,
                                          ProtocolHelperValuePool *valuePool)
{
    Item item;
    item.setId(data.id());
    item.setRevision(data.revision());
    if (!fetchScope || fetchScope->fetchRemoteIdentification()) {
        item.setRemoteId(data.remoteId());
        item.setRemoteRevision(data.remoteRevision());
    }
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

    if ((!fetchScope || fetchScope->fetchTags()) && !data.tags().isEmpty()) {
        Tag::List tags;
        tags.reserve(data.tags().size());
        Q_FOREACH (const Protocol::FetchTagsResponse &tag, data.tags()) {
            tags.append(parseTagFetchResult(tag));
        }
        item.setTags(tags);
    }

    if ((!fetchScope || fetchScope->fetchRelations()) && !data.relations().isEmpty()) {
        Relation::List relations;
        relations.reserve(data.relations().size());
        Q_FOREACH (const Protocol::FetchRelationsResponse &rel, data.relations()) {
            relations.append(parseRelationFetchResult(rel));
        }
        item.d_ptr->mRelations = relations;
    }

    if ((!fetchScope || fetchScope->fetchVirtualReferences()) && !data.virtualReferences().isEmpty()) {
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
    item.setModificationTime(data.mTime());
    parseAncestorsCached(data.ancestors(), &item, data.parentId(), valuePool);
    Q_FOREACH (const Protocol::StreamPayloadResponse &part, data.parts()) {
        ProtocolHelper::PartNamespace ns;
        const QByteArray plainKey = decodePartIdentifier(part.payloadName(), ns);
        const auto metaData = part.metaData();
        switch (ns) {
        case ProtocolHelper::PartPayload:
            if (fetchScope && !fetchScope->fullPayload() && !fetchScope->payloadParts().contains(plainKey)) {
                continue;
            }
            ItemSerializer::deserialize(item, plainKey, part.data(), metaData.version(), static_cast<ItemSerializer::PayloadStorage>(metaData.storageType()));
            if (metaData.storageType() == Protocol::PartMetaData::Foreign) {
                item.d_ptr->mPayloadPath = QString::fromUtf8(part.data());
            }
            break;
        case ProtocolHelper::PartAttribute: {
            if (fetchScope && !fetchScope->allAttributes() && !fetchScope->attributes().contains(plainKey)) {
                continue;
            }
            Attribute *attr = AttributeFactory::createAttribute(plainKey);
            Q_ASSERT(attr);
            if (metaData.storageType() == Protocol::PartMetaData::External) {
                const QString filename = ExternalPartStorage::resolveAbsolutePath(part.data());
                QFile file(filename);
                if (file.open(QFile::ReadOnly)) {
                    attr->deserialize(file.readAll());
                } else {
                    qCWarning(AKONADICORE_LOG) << "Failed to open attribute file: " << filename;
                    delete attr;
                    attr = nullptr;
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
    tag.d_ptr->resetChangeLog();
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
    // qCDebug(AKONADICORE_LOG) << filePath << fileName;
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
    // qCDebug(AKONADICORE_LOG) << "Wrote" << data.size() << "bytes to " << file.fileName();

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
