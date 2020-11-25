/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "dbinitializer.h"
#include "akonadiserver_debug.h"

#include <storage/querybuilder.h>
#include <storage/datastore.h>
#include <storage/parttypehelper.h>

#include "shared/akranges.h"

using namespace Akonadi;
using namespace Akonadi::Server;
using namespace AkRanges;

DbInitializer::~DbInitializer()
{
    cleanup();
}

Resource DbInitializer::createResource(const char *name)
{
    Resource res;
    qint64 id = -1;
    res.setName(QLatin1String(name));
    const bool ret = res.insert(&id);
    Q_ASSERT(ret);
    Q_UNUSED(ret)
    mResource = res;
    return res;
}

Collection DbInitializer::createCollection(const char *name, const Collection &parent)
{
    Collection col;
    if (parent.isValid()) {
        col.setParent(parent);
    }
    col.setName(QLatin1String(name));
    col.setRemoteId(QLatin1String(name));
    col.setResource(mResource);
    const bool ret = col.insert();
    Q_ASSERT(ret);
    Q_UNUSED(ret)
    return col;
}

PimItem DbInitializer::createItem(const char *name, const Collection &parent)
{
    PimItem item;
    MimeType mimeType = MimeType::retrieveByName(QStringLiteral("test"));
    if (!mimeType.isValid()) {
        MimeType mt;
        mt.setName(QStringLiteral("test"));
        mt.insert();
        mimeType = mt;
    }
    item.setMimeType(mimeType);
    item.setCollection(parent);
    item.setRemoteId(QLatin1String(name));
    const bool ret = item.insert() ;
    Q_ASSERT(ret);
    Q_UNUSED(ret)
    return item;
}

Part DbInitializer::createPart(qint64 pimItem, const QByteArray &partName, const QByteArray &partData)
{
    auto partType = PartTypeHelper::parseFqName(QString::fromLatin1(partName));
    PartType type = PartType::retrieveByFQNameOrCreate(partType.first, partType.second);

    Part part;
    part.setPimItemId(pimItem);
    part.setPartTypeId(type.id());
    part.setData(partData);
    part.setDatasize(partData.size());
    const bool ret = part.insert();
    Q_ASSERT(ret);
    Q_UNUSED(ret)
    return part;
}

QByteArray DbInitializer::toByteArray(bool enabled)
{
    if (enabled) {
        return "TRUE";
    }
    return "FALSE";
}

QByteArray DbInitializer::toByteArray(Collection::Tristate tristate)
{

    switch (tristate) {
    case Collection::True:
        return "TRUE";
    case Collection::False:
        return "FALSE";
    case Collection::Undefined:
    default:
        break;
    }
    return "DEFAULT";
}

Akonadi::Protocol::FetchCollectionsResponsePtr DbInitializer::listResponse(const Collection &col,
                                                                           bool ancestors,
                                                                           bool mimetypes,
                                                                           const QStringList &ancestorFetchScope)
{
    auto resp = Akonadi::Protocol::FetchCollectionsResponsePtr::create(col.id());
    resp->setParentId(col.parentId());
    resp->setName(col.name());
    if (mimetypes) {
        resp->setMimeTypes(col.mimeTypes() | Views::transform(&MimeType::name) | Actions::toQList);
    }
    resp->setRemoteId(col.remoteId());
    resp->setRemoteRevision(col.remoteRevision());
    resp->setResource(col.resource().name());
    resp->setIsVirtual(col.isVirtual());
    Akonadi::Protocol::CachePolicy cp;
    cp.setInherit(true);
    cp.setLocalParts({ QLatin1String("ALL") });
    resp->setCachePolicy(cp);
    if (ancestors) {
        QVector<Akonadi::Protocol::Ancestor> ancs;
        Collection parent = col.parent();
        while (parent.isValid()) {
            Akonadi::Protocol::Ancestor anc;
            anc.setId(parent.id());
            anc.setRemoteId(parent.remoteId());
            anc.setName(parent.name());
            if (!ancestorFetchScope.isEmpty()) {
                anc.setRemoteId(parent.remoteId());
                Akonadi::Protocol::Attributes attrs;
                Q_FOREACH(const CollectionAttribute &attr, parent.attributes()) {
                    if (ancestorFetchScope.contains(QString::fromLatin1(attr.type()))) {
                        attrs.insert(attr.type(), attr.value());
                    }
                }
                attrs.insert("ENABLED", parent.enabled() ? "TRUE" : "FALSE");
                anc.setAttributes(attrs);
            }
            parent = parent.parent();
            ancs.push_back(anc);
        }
        // Root
        ancs.push_back(Akonadi::Protocol::Ancestor(0));
        resp->setAncestors(ancs);
    }
    resp->setEnabled(col.enabled());
    resp->setDisplayPref(static_cast<Tristate>(col.displayPref()));
    resp->setSyncPref(static_cast<Tristate>(col.syncPref()));
    resp->setIndexPref(static_cast<Tristate>(col.indexPref()));

    Akonadi::Protocol::Attributes attrs;
    Q_FOREACH(const CollectionAttribute &attr, col.attributes()) {
        attrs.insert(attr.type(), attr.value());
    }
    resp->setAttributes(attrs);
    return resp;
}

Akonadi::Protocol::FetchItemsResponsePtr DbInitializer::fetchResponse(const PimItem &item)
{
    auto resp = Akonadi::Protocol::FetchItemsResponsePtr::create();
    resp->setId(item.id());
    resp->setRevision(item.rev());
    resp->setMimeType(item.mimeType().name());
    resp->setRemoteId(item.remoteId());
    resp->setParentId(item.collectionId());
    resp->setSize(item.size());
    resp->setMTime(item.datetime());
    resp->setRemoteRevision(item.remoteRevision());
    resp->setGid(item.gid());
    const auto flags = item.flags();
    QVector<QByteArray> flagNames;
    for (const auto &flag : flags) {
        flagNames.push_back(flag.name().toUtf8());
    }
    resp->setFlags(flagNames);

    return resp;
}

Collection DbInitializer::collection(const char *name)
{
    return Collection::retrieveByName(QLatin1String(name));
}

void DbInitializer::cleanup()
{
    Q_FOREACH (Collection col, mResource.collections()) { //krazy:exclude=foreach
        if (!col.isVirtual()) {
            col.remove();
        }
    }
    mResource.remove();

    if (DataStore::self()->database().isOpen()) {
        {
            QueryBuilder qb(Relation::tableName(), QueryBuilder::Delete);
            qb.exec();
        }
        {
            QueryBuilder qb(Tag::tableName(), QueryBuilder::Delete);
            qb.exec();
        }
        {
            QueryBuilder qb(TagType::tableName(), QueryBuilder::Delete);
            qb.exec();
        }
    }

    Q_FOREACH(Part part, Part::retrieveAll()) {          //krazy:exclude=foreach
        part.remove();
    }
    Q_FOREACH(PimItem item, PimItem::retrieveAll()) {    //krazy:exclude=foreach
        item.remove();
    }
}
