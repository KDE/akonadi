/*
  SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "fakeakonadiservercommand.h"

#include <QMetaMethod>
#include <QStringList>

#include "akranges.h"
#include "entitydisplayattribute.h"
#include "fakeserverdata.h"
#include "tagattribute.h"

using namespace Akonadi;
using namespace AkRanges;

FakeAkonadiServerCommand::FakeAkonadiServerCommand(FakeAkonadiServerCommand::Type type, FakeServerData *serverData)
    : m_type(type)
    , m_serverData(serverData)
    , m_model(serverData->model())
{
    connectForwardingSignals();
}

bool FakeAkonadiServerCommand::isTagSignal(const QByteArray &signal) const
{
    return signal.startsWith("emit_tag") || signal.startsWith("emit_monitoredTag");
}

bool FakeAkonadiServerCommand::isItemSignal(const QByteArray &signal) const
{
    return signal.startsWith("emit_item") || signal.startsWith("emit_monitoredItem");
}

bool FakeAkonadiServerCommand::isCollectionSignal(const QByteArray &signal) const
{
    return signal.startsWith("emit_collection") || signal.startsWith("emit_monitoredCollection");
}

void FakeAkonadiServerCommand::connectForwardingSignals()
{
    const auto mo = FakeAkonadiServerCommand::metaObject();
    for (int methodIndex = 0; methodIndex < mo->methodCount(); ++methodIndex) {
        const QMetaMethod mm = mo->method(methodIndex);
        const QByteArray signature = mm.methodSignature();
        if (mm.methodType() == QMetaMethod::Signal) {
            if ((qobject_cast<TagModel *>(m_model) && isTagSignal(signature))
                || (qobject_cast<EntityTreeModel *>(m_model) && (isCollectionSignal(signature) || isItemSignal(signature)))) {
                const int modelSlotIndex = m_model->metaObject()->indexOfSlot(signature.mid(5).constData());
                if (modelSlotIndex < 0) {
                    qWarning() << "Slot not found in" << m_model->metaObject()->className() << ":" << signature.mid(5).constData();
                    Q_ASSERT(modelSlotIndex >= 0);
                }
                mo->connect(this, methodIndex, m_model, modelSlotIndex);
            }
        }
    }
}

Collection FakeAkonadiServerCommand::getCollectionByDisplayName(const QString &displayName) const
{
    Q_ASSERT(qobject_cast<EntityTreeModel *>(m_model));
    QModelIndexList list = m_model->match(m_model->index(0, 0), Qt::DisplayRole, displayName, 1, Qt::MatchRecursive);
    if (list.isEmpty()) {
        return Collection();
    }
    return list.first().data(EntityTreeModel::CollectionRole).value<Collection>();
}

Item FakeAkonadiServerCommand::getItemByDisplayName(const QString &displayName) const
{
    Q_ASSERT(qobject_cast<EntityTreeModel *>(m_model));
    QModelIndexList list = m_model->match(m_model->index(0, 0), Qt::DisplayRole, displayName, 1, Qt::MatchRecursive);
    if (list.isEmpty()) {
        return Item();
    }
    return list.first().data(EntityTreeModel::ItemRole).value<Item>();
}

Tag FakeAkonadiServerCommand::getTagByDisplayName(const QString &displayName) const
{
    Q_ASSERT(qobject_cast<TagModel *>(m_model));
    QModelIndexList list = m_model->match(m_model->index(0, 0), Qt::DisplayRole, displayName, 1, Qt::MatchRecursive);
    if (list.isEmpty()) {
        return Tag();
    }

    return list.first().data(TagModel::TagRole).value<Tag>();
}

void FakeJobResponse::doCommand()
{
    if (m_type == RespondToCollectionFetch) {
        Q_EMIT emit_collectionsFetched(m_collections | Views::values | Actions::toQVector);
    } else if (m_type == RespondToItemFetch) {
        setProperty("FetchCollectionId", m_parentCollection.id());
        Q_EMIT emit_itemsFetched(m_items | Views::values | Actions::toQVector);
    } else if (m_type == RespondToTagFetch) {
        Q_EMIT emit_tagsFetched(m_tags | Views::values | Actions::toQVector);
    }
}

QList<FakeJobResponse::Token> FakeJobResponse::tokenize(const QString &treeString)
{
    QStringList parts = treeString.split(QLatin1Char('-'));

    QList<Token> tokens;
    const QStringList::const_iterator begin = parts.constBegin();
    const QStringList::const_iterator end = parts.constEnd();

    QStringList::const_iterator it = begin;
    ++it;
    for (; it != end; ++it) {
        Token token;
        if (it->trimmed().isEmpty()) {
            token.type = Token::Branch;
        } else {
            token.type = Token::Leaf;
            token.content = it->trimmed();
        }
        tokens.append(token);
    }
    return tokens;
}

QList<FakeAkonadiServerCommand *> FakeJobResponse::interpret(FakeServerData *fakeServerData, const QString &serverData)
{
    QList<FakeAkonadiServerCommand *> list;
    const QList<FakeJobResponse *> response = parseTreeString(fakeServerData, serverData);

    for (FakeJobResponse *command : response) {
        list.append(command);
    }
    return list;
}

QList<FakeJobResponse *> FakeJobResponse::parseTreeString(FakeServerData *fakeServerData, const QString &treeString)
{
    int depth = 0;

    QList<FakeJobResponse *> collectionResponseList;
    QHash<Collection::Id, FakeJobResponse *> itemResponseMap;
    QList<FakeJobResponse *> tagResponseList;

    Collection::List recentCollections;
    Tag::List recentTags;

    recentCollections.append(Collection::root());
    recentTags.append(Tag());

    QList<Token> tokens = tokenize(treeString);
    while (!tokens.isEmpty()) {
        Token token = tokens.takeFirst();

        if (token.type == Token::Branch) {
            ++depth;
            continue;
        }
        Q_ASSERT(token.type == Token::Leaf);
        parseEntityString(collectionResponseList, itemResponseMap, tagResponseList, recentCollections, recentTags, fakeServerData, token.content, depth);

        depth = 0;
    }
    return collectionResponseList + tagResponseList;
}

void FakeJobResponse::parseEntityString(QList<FakeJobResponse *> &collectionResponseList,
                                        QHash<Collection::Id, FakeJobResponse *> &itemResponseMap,
                                        QList<FakeJobResponse *> &tagResponseList,
                                        Collection::List &recentCollections,
                                        Tag::List &recentTags,
                                        FakeServerData *fakeServerData,
                                        const QString &_entityString,
                                        int depth)
{
    QString entityString = _entityString;
    if (entityString.startsWith(QLatin1Char('C'))) {
        Collection collection;
        entityString.remove(0, 2);
        Q_ASSERT(entityString.startsWith(QLatin1Char('(')));
        entityString.remove(0, 1);
        QStringList parts = entityString.split(QLatin1Char(')'));

        if (!parts.first().isEmpty()) {
            QString typesString = parts.takeFirst();

            QStringList types = typesString.split(QLatin1Char(','));
            types.replaceInStrings(QStringLiteral(" "), QLatin1String(""));
            collection.setContentMimeTypes(types);
        } else {
            parts.removeFirst();
        }

        collection.setId(fakeServerData->nextCollectionId());
        collection.setName(QStringLiteral("Collection %1").arg(collection.id()));
        collection.setRemoteId(QStringLiteral("remoteId %1").arg(collection.id()));

        if (depth == 0) {
            collection.setParentCollection(Collection::root());
        } else {
            collection.setParentCollection(recentCollections.at(depth));
        }

        if (recentCollections.size() == (depth + 1)) {
            recentCollections.append(collection);
        } else {
            recentCollections[depth + 1] = collection;
        }

        int order = 0;
        if (!parts.first().isEmpty()) {
            QString displayName;
            QString optionalSection = parts.first().trimmed();
            if (optionalSection.startsWith(QLatin1Char('\''))) {
                optionalSection.remove(0, 1);
                QStringList optionalParts = optionalSection.split(QLatin1Char('\''));
                displayName = optionalParts.takeFirst();
                auto eda = new EntityDisplayAttribute();
                eda->setDisplayName(displayName);
                collection.addAttribute(eda);
                optionalSection = optionalParts.first();
            }

            QString orderString = optionalSection.trimmed();
            if (!orderString.isEmpty()) {
                bool ok;
                order = orderString.toInt(&ok);
                Q_ASSERT(ok);
            }
        } else {
            order = 1;
        }
        while (collectionResponseList.size() < order) {
            collectionResponseList.append(new FakeJobResponse(recentCollections[depth], FakeJobResponse::RespondToCollectionFetch, fakeServerData));
        }
        collectionResponseList[order - 1]->appendCollection(collection);
    }
    if (entityString.startsWith(QLatin1Char('I'))) {
        Item item;
        int order = 0;
        entityString.remove(0, 2);
        entityString = entityString.trimmed();
        QString type;
        int iFirstSpace = entityString.indexOf(QLatin1Char(' '));
        type = entityString.left(iFirstSpace);
        entityString = entityString.remove(0, iFirstSpace + 1).trimmed();
        if (iFirstSpace > 0 && !entityString.isEmpty()) {
            QString displayName;
            QString optionalSection = entityString;
            if (optionalSection.startsWith(QLatin1Char('\''))) {
                optionalSection.remove(0, 1);
                QStringList optionalParts = optionalSection.split(QLatin1Char('\''));
                displayName = optionalParts.takeFirst();
                auto eda = new EntityDisplayAttribute();
                eda->setDisplayName(displayName);
                item.addAttribute(eda);
                optionalSection = optionalParts.first();
            }
            QString orderString = optionalSection.trimmed();
            if (!orderString.isEmpty()) {
                bool ok;
                order = orderString.toInt(&ok);
                Q_ASSERT(ok);
            }
        } else {
            type = entityString;
        }
        Q_UNUSED(order)

        item.setMimeType(type);
        item.setId(fakeServerData->nextItemId());
        item.setRemoteId(QStringLiteral("RId_%1 %2").arg(item.id()).arg(type));
        item.setParentCollection(recentCollections.at(depth));

        Collection::Id colId = recentCollections[depth].id();
        if (!itemResponseMap.contains(colId)) {
            auto newResponse = new FakeJobResponse(recentCollections[depth], FakeJobResponse::RespondToItemFetch, fakeServerData);
            itemResponseMap.insert(colId, newResponse);
            collectionResponseList.append(newResponse);
        }
        itemResponseMap[colId]->appendItem(item);
    }
    if (entityString.startsWith(QLatin1Char('T'))) {
        Tag tag;
        int order = 0;
        entityString.remove(0, 2);
        entityString = entityString.trimmed();
        int iFirstSpace = entityString.indexOf(QLatin1Char(' '));
        QString type = entityString.left(iFirstSpace);
        entityString = entityString.remove(0, iFirstSpace + 1).trimmed();
        tag.setType(type.toLatin1());

        if (iFirstSpace > 0 && !entityString.isEmpty()) {
            QString displayName;
            QString optionalSection = entityString;
            if (optionalSection.startsWith(QLatin1Char('\''))) {
                optionalSection.remove(0, 1);
                QStringList optionalParts = optionalSection.split(QLatin1Char('\''));
                displayName = optionalParts.takeFirst();
                auto ta = new TagAttribute();
                ta->setDisplayName(displayName);
                tag.addAttribute(ta);
                optionalSection = optionalParts.first();
            }
            QString orderString = optionalSection.trimmed();
            if (!orderString.isEmpty()) {
                bool ok;
                order = orderString.toInt(&ok);
                Q_ASSERT(ok);
            }
        } else {
            type = entityString;
        }

        tag.setId(fakeServerData->nextTagId());
        tag.setRemoteId("RID_" + QByteArray::number(tag.id()) + ' ' + type.toLatin1());
        tag.setType(type.toLatin1());

        if (depth == 0) {
            tag.setParent(Tag());
        } else {
            tag.setParent(recentTags.at(depth));
        }

        if (recentTags.size() == (depth + 1)) {
            recentTags.append(tag);
        } else {
            recentTags[depth + 1] = tag;
        }

        while (tagResponseList.size() < order) {
            tagResponseList.append(new FakeJobResponse(recentTags[depth], FakeJobResponse::RespondToTagFetch, fakeServerData));
        }
        tagResponseList[order - 1]->appendTag(tag);
    }
}

void FakeCollectionMovedCommand::doCommand()
{
    Collection collection = getCollectionByDisplayName(m_collectionName);
    Collection source = getCollectionByDisplayName(m_sourceName);
    Collection target = getCollectionByDisplayName(m_targetName);

    Q_ASSERT(collection.isValid());
    Q_ASSERT(source.isValid());
    Q_ASSERT(target.isValid());

    collection.setParentCollection(target);

    Q_EMIT emit_monitoredCollectionMoved(collection, source, target);
}

void FakeCollectionAddedCommand::doCommand()
{
    Collection parent = getCollectionByDisplayName(m_parentName);

    Q_ASSERT(parent.isValid());

    Collection collection;
    collection.setId(m_serverData->nextCollectionId());
    collection.setName(QStringLiteral("Collection %1").arg(collection.id()));
    collection.setRemoteId(QStringLiteral("remoteId %1").arg(collection.id()));
    collection.setParentCollection(parent);

    auto eda = new EntityDisplayAttribute();
    eda->setDisplayName(m_collectionName);
    collection.addAttribute(eda);

    Q_EMIT emit_monitoredCollectionAdded(collection, parent);
}

void FakeCollectionRemovedCommand::doCommand()
{
    Collection collection = getCollectionByDisplayName(m_collectionName);

    Q_ASSERT(collection.isValid());

    Q_EMIT emit_monitoredCollectionRemoved(collection);
}

void FakeCollectionChangedCommand::doCommand()
{
    if (m_collection.isValid()) {
        Q_EMIT emit_monitoredCollectionChanged(m_collection);
        return;
    }
    Collection collection = getCollectionByDisplayName(m_collectionName);
    Collection parent = getCollectionByDisplayName(m_parentName);

    Q_ASSERT(collection.isValid());

    Q_EMIT emit_monitoredCollectionChanged(collection);
}

void FakeItemMovedCommand::doCommand()
{
    Item item = getItemByDisplayName(m_itemName);
    Collection source = getCollectionByDisplayName(m_sourceName);
    Collection target = getCollectionByDisplayName(m_targetName);

    Q_ASSERT(item.isValid());
    Q_ASSERT(source.isValid());
    Q_ASSERT(target.isValid());

    item.setParentCollection(target);

    Q_EMIT emit_monitoredItemMoved(item, source, target);
}

void FakeItemAddedCommand::doCommand()
{
    Collection parent = getCollectionByDisplayName(m_parentName);

    Q_ASSERT(parent.isValid());

    Item item;
    item.setId(m_serverData->nextItemId());
    item.setRemoteId(QStringLiteral("remoteId %1").arg(item.id()));
    item.setParentCollection(parent);

    auto eda = new EntityDisplayAttribute();
    eda->setDisplayName(m_itemName);
    item.addAttribute(eda);

    Q_EMIT emit_monitoredItemAdded(item, parent);
}

void FakeItemRemovedCommand::doCommand()
{
    Item item = getItemByDisplayName(m_itemName);

    Q_ASSERT(item.isValid());

    Q_EMIT emit_monitoredItemRemoved(item);
}

void FakeItemChangedCommand::doCommand()
{
    Item item = getItemByDisplayName(m_itemName);
    Collection parent = getCollectionByDisplayName(m_parentName);

    Q_ASSERT(item.isValid());
    Q_ASSERT(parent.isValid());

    Q_EMIT emit_monitoredItemChanged(item, QSet<QByteArray>());
}

void FakeTagAddedCommand::doCommand()
{
    const Tag parent = getTagByDisplayName(m_parentName);

    Tag tag;
    tag.setId(m_serverData->nextTagId());
    tag.setName(m_tagName);
    tag.setRemoteId("remoteId " + QByteArray::number(tag.id()));
    tag.setParent(parent);

    Q_EMIT emit_monitoredTagAdded(tag);
}

void FakeTagChangedCommand::doCommand()
{
    const Tag tag = getTagByDisplayName(m_tagName);

    Q_ASSERT(tag.isValid());

    Q_EMIT emit_monitoredTagChanged(tag);
}

void FakeTagMovedCommand::doCommand()
{
    Tag tag = getTagByDisplayName(m_tagName);
    Tag newParent = getTagByDisplayName(m_newParent);

    Q_ASSERT(tag.isValid());

    tag.setParent(newParent);

    Q_EMIT emit_monitoredTagChanged(tag);
}

void FakeTagRemovedCommand::doCommand()
{
    const Tag tag = getTagByDisplayName(m_tagName);

    Q_ASSERT(tag.isValid());

    Q_EMIT emit_monitoredTagRemoved(tag);
}

#include "moc_fakeakonadiservercommand.cpp"
