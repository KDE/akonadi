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

#include "attributefactory.h"
#include "collectionstatistics.h"
#include "entity_p.h"
#include "item_p.h"
#include "exception.h"
#include "itemserializer_p.h"
#include "itemserializerplugin.h"
#include "servermanager.h"
#include "tagfetchscope.h"
#include <akonadi/private/xdgbasedirs_p.h>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QVarLengthArray>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>

#include <qdebug.h>
#include <klocalizedstring.h>

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

void ProtocolHelper::parseAncestorsCached(const QVector<Protocol::Ancestor> &ancestors, Entity *entity,
                                          Collection::Id parentCollection,
                                          ProtocolHelperValuePool *pool)
{
    if (!pool || parentCollection == -1) {
        // if no pool or parent collection id is provided we can't cache anything, so continue as usual
        parseAncestors(ancestors, entity);
        return;
    }

    if (pool->ancestorCollections.contains(parentCollection)) {
        // ancestor chain is cached already, so use the cached value
        entity->setParentCollection(pool->ancestorCollections.value(parentCollection));
    } else {
        // not cached yet, parse the chain
        parseAncestors(data, entity);
        pool->ancestorCollections.insert(parentCollection, entity->parentCollection());
    }
}

void ProtocolHelper::parseAncestors(const QVector<Protocol::Ancestor> &ancestors, Entity *entity)
{
    static const Collection::Id rootCollectionId = Collection::root().id();
    QList<QByteArray> parentIds;

    Entity *current = entity;
    for (const Protocol::Ancestor &ancestor : ancestors) {
        if (ancestor.id() == rootCollectionId) {
            current->setParentCollection(Collection::root());
            break;
        }

        Akonadi::Collection parentCollection(ancestor.id());
        parentCollection.setName(ancestor.name());
        parentCollection.setRemoteId(ancestor.remoteId());
        parseAttributes(ancestor.attributes(), &parentCollection);
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
}

CollectionStatistics ProtocolHelper::parseCollectionStatistics(const Protocol::FetchCollectionStatsResponse &stats)
{
    CollectionStatistics cs;
    cs.setCount(stats.count());
    cs.setSize(stats.size());
    cs.setUnreadCount(stats.unseen());
    return cs;
}

template<typename T>
inline static void parseAttributesImpl(const Protocol::Attributes &attributes, T *entity)
{
    for (auto iter = attributes.cbegin(), end = attributes.cend(); iter != end; ++iter) {
        Attribute *attribute = AttributeFactory::createAttribute(iter.key());
        if (!attribute) {
            qWarning() << "Warning: unknown attribute" << iter.key();
            continue;
        }
        attribute->deserialize(iter.value());
        entity->addAttribute(attribute);
    }
}

void ProtocolHelper::parseAttributes(const Protocol::Attributes &attributes, Entity *entity)
{
    parseAttributesImpl(attributes, entity);
}

void ProtocolHelper::parseAttributes(const Protocol::Attributes &attributes, AttributeEntity *entity)
{
    parseAttributesImpl(attributes, entity);
}


template<typename T>
inline static Protocol::Attributes attributesToProtocolImpl(const T &entity, bool ns)
{
    Protocol::Attributes attributes;
    for (const Attribute *attr : entity.attributes()) {
        attributes.insert(ProtocolHelper::encodePartIdentifier(ns ? ProtocolHelper::PartAttribute : ProtocolHelper::PartGlobal, attr->type()),
                          attr->serialized());
    }
    return attributes;
}

Protocol::Attributes ProtocolHelper::attributesToProtocol(const Entity &entity, bool ns)
{
    return attributesToProtocolImpl(entity, ns);
}

Protocol::Attributes ProtocolHelper::attributesToProtocol(const AttributeEntity &entity, bool ns)
{
    return attributesToProtocolImpl(entity, ns);
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

    parseAttributes(data.attributes(), &collection);

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

Scope ProtocolHelper::entitySetToScope(const QList<Item> &_objects, const QByteArray &command)
{
    if (_objects.isEmpty()) {
        throw Exception("No objects specified");
    }

    Item::List objects(_objects);
    std::sort(objects.begin(), objects.end(), boost::bind(&Item::id, _1) < boost::bind(&Item::id, _2));
    if (objects.first().isValid()) {
        // all items have a uid set
        return entitySetToScope<Item>(objects, command);
    }
    // check if all items have a gid
    if (std::find_if(objects.constBegin(), objects.constEnd(),
                     boost::bind(&QString::isEmpty, boost::bind(&Item::gid, _1)))
        == objects.constEnd()) {
        QStringList gids;
        gids.reserve(objects.count());
        for (const Item &object : objects) {
            gids << object.gid();
        }

        return Scope(Scope::Gid, gids);
    }
    return entitySetToScope<Item>(objects, command);
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
        return Scope(Scope::HierarchicalRid, { QString() });
    }
    if (col.remoteId().isEmpty()) {
        return Scope();
    }

    QStringList chain;
    Collection c = col;
    while (!c.remoteId().isEmpty()) {
        chain.append(c.remoteId());
        c = c.parentCollection();
    }
    return Scope(Scope::HierarchicalRid, chain);
}

Scope ProtocolHelper::hierarchicalRidToScope(const Item &item)
{
    return Scope(Scope::HierarchicalRid, item.remoteId() + hierarchicalRidToScope(item.parentCollection()));
}

Protocol::FetchScope ProtocolHelper::itemFetchScopeProtocol(const ItemFetchScope &fetchScope)
{
    Protocol::FetchScope fs;
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
        fs.setFetch(Protocol::FetchScope::fetchTags());
        if (!fetchScope.tagFetchScope().fetchIdOnly()) {
            fs.setTagFetchScope(fetchScope.tagFetchScope().attributes());
        }
    }

    fs.setFetch(Protocol::FetchScope::VirtReferences, fetchScope.fetchVirtualReferences());
    fs.setFetch(Protocol::FetchScope::MTime, fetchScope.fetchModificationTime());
    fs.setFetch(Protocol::FetchScope::Relations, fetchScope.fetchRelations());

    foreach (const QByteArray &part, fetchScope.payloadParts()) {
        fs.setRequestedPayloads(QString::fromLatin1(ProtocolHelper::encodePartIdentifier(ProtocolHelper::PartPayload, part)));
    }
    foreach (const QByteArray &part, fetchScope.attributes()) {
        fs.setRequestedParts(QString::fromLatin1(ProtocolHelper::encodePartIdentifier(ProtocolHelper::PartAttribute, part)));
    }

    return fs;
}

static Item::Flags convertFlags(const QList<QByteArray> &flags, ProtocolHelperValuePool *valuePool)
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
    foreach (const QByteArray &flag, flags) {
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
    item.setGid(data.gid());
    item.setStorageCollectionId(data.parentId());

    if (valuePool) {
        item.setMimeType(valuePool->mimeTypePool.sharedValue(data.mimeType());
    } else {
        item.setMimeType(data.mimeType());
    }

    if (!item.isValid()) {
        return;
    }

    item.setFlag(convertFlags(data.flags(), valuePool));

    if (!data.tags().isEmpty()) {
        Tag::List tags;
        tags.reserve(data.tags().size());
        for (const Protocol::FetchTagsResponse &tag : data.tags()) {
            tags.append(parseTagFetchResult(tag));
        }
        item.setTags(tags);
    }

    if (!data.relations().isEmpty()) {
        Relation::List relations;
        relations.reserve(data.relations().size());
        for (const Protocol::FetchRelationsResponse &rel : data.relations()) {
            relations.append(parseRelationFetchResult(rel));
        }
        item.d_func()->mRelations = relations;
    }

    if (!data.virtualReferences().isEmpty()) {
        Collection::List virtRefs;
        virtRefs.reserve(data.virtualReferences().size());
        for (qint64 colId : data.virtualReferences()) {
            virtRefs.append(Collection(colId));
        }
        item.setVirtualReferences(virtRefs);
    }

    if (!data.cachedParts().isEmpty()) {
        QSet<QByteArray> cp;
        cp.reserve(data.cachedParts().size());
        for (const QByteArray &ba : data.cachedParts()) {
            cp.insert(ba);
        }
        item.setCachedPayloadParts(cp);
    }

    item.setSize(data.size());
    item.setModificationTime(data.MTime());
    parseAncestorsCached(data.ancestors(), &item, data.parentId(), valuePool);

    const auto parts = data.parts();
    for (auto iter = parts.constBegin(), end = parts.constEnd(); iter != end; ++iter) {
        ProtocolHelper::PartNamespace ns;
        const QByteArray plainKey = decodePartIdentifier(iter.key().name(), ns);
        switch (ns) {
        case ProtocolHelper::PartPayload:
            ItemSerializer::deserialize(item, plainKey, iter.value().data(), iter.key().version(), iter.value().isExternal());
            break;
        case ProtocolHelper::PartAttribute: {
            Attribute *attr = AttributeFactory::createAttribute(plainKey);
            Q_ASSERT(attr);
            if (iter.value().isExternal()) {
                const QString filename = ProtocolHelper::absolutePayloadFilePath(QString::fromUtf8(iter.value().data()));
                QFile file(QString::fromUtf8(filename);
                if (file.open(QFile::ReadOnly)) {
                    attr->deserialize(file.readAll());
                } else {
                    qWarning() << "Failed to open attribute file: " << filename;
                    delete attr;
                    attr = 0;
                }
            } else {
                attr->deserialize(iter.value().data());
            }
            if (attr) {
                item.addAttribute(attr);
            }
            break;
        }
        case ProtocolHelper::PartGlobal:
        default:
            qWarning() << "Unknown item part type:" << iter.key().name();
        }
    }

    item.d_ptr->resetChangeLog();
}

Tag ProtocolHelper::parseTagFetchResult(const Protocol::FetchTagsResponse &data)
{
    Tag tag;
    tag.setId(data.id());
    tag.setGid(data.gid());
    tag.setRemoteId(data.remoteId());
    tag.setType(data.type());

    parseAttributes(data.attributes(), &tag);
    return tag;
}

Relation ProtocolHelper::parseRelationFetchResult(const Protocol::FetchRelationsResponse &data)
{
    Relation relation;
    relation.setLeft(data.left());
    relation.setRight(data.right());
    relation.setRemoteId(data.remoteId());
    relation.setType(data.type());
    return relation;
}

QString ProtocolHelper::akonadiStoragePath()
{
    QString fullRelPath = QLatin1String("akonadi");
    if (Akonadi::ServerManager::hasInstanceIdentifier()) {
        fullRelPath += QDir::separator() + QLatin1String("instance") + QDir::separator() + Akonadi::ServerManager::instanceIdentifier();
    }
    return XdgBaseDirs::saveDir("data", fullRelPath);
}

QString ProtocolHelper::absolutePayloadFilePath(const QString &fileName)
{
    QFileInfo fi(fileName);
    if (!fi.isAbsolute()) {
        return akonadiStoragePath() + QDir::separator() + QLatin1String("file_db_data") + QDir::separator() + fileName;
    }

    return fileName;
}

bool ProtocolHelper::streamPayloadToFile(const QByteArray &command, const QByteArray &data, QByteArray &error)
{
    const int fnStart = command.indexOf("[FILE ") + 6;
    if (fnStart == -1) {
        qDebug() << "Unexpected response";
        return false;
    }
    const int fnEnd = command.indexOf("]", fnStart);
    const QByteArray fn = command.mid(fnStart, fnEnd - fnStart);
    const QString fileName = ProtocolHelper::absolutePayloadFilePath(QString::fromLatin1(fn));
    if (!fileName.startsWith(akonadiStoragePath())) {
        qWarning() << "Invalid file path" << fileName;
        error = "Invalid file path";
        return false;
    }
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Failed to open destination payload file" << file.errorString();
        error = "Failed to store payload into file";
        return false;
    }
    if (file.write(data) != data.size()) {
        qWarning() << "Failed to write all payload data to file";
        error = "Failed to store payload into file";
        return false;
    }
    qDebug() << "Wrote" << data.size() << "bytes to " << file.fileName();

    // Make sure stuff is written to disk
    file.close();
    return true;
}

Akonadi::Tristate ProtocolHelper::listPreference(Collection::ListPreference pref)
{
    switch (pref)
    {
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
