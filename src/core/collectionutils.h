/*
    SPDX-FileCopyrightText: 2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QStringList>

#include "collectionstatistics.h"
#include "entitydisplayattribute.h"
#include "item.h"

namespace Akonadi
{
/**
 * @internal
 */
namespace CollectionUtils
{
Q_REQUIRED_RESULT inline bool isVirtualParent(const Collection &collection)
{
    return (collection.parentCollection() == Collection::root() && collection.isVirtual());
}

Q_REQUIRED_RESULT inline bool isReadOnly(const Collection &collection)
{
    return !(collection.rights() & Collection::CanCreateItem);
}

Q_REQUIRED_RESULT inline bool isRoot(const Collection &collection)
{
    return (collection == Collection::root());
}

Q_REQUIRED_RESULT inline bool isResource(const Collection &collection)
{
    return (collection.parentCollection() == Collection::root());
}

Q_REQUIRED_RESULT inline bool isStructural(const Collection &collection)
{
    return collection.contentMimeTypes().isEmpty();
}

Q_REQUIRED_RESULT inline bool isFolder(const Collection &collection)
{
    return (!isRoot(collection) && !isResource(collection) && !isStructural(collection) && collection.resource() != QLatin1String("akonadi_search_resource"));
}

Q_REQUIRED_RESULT inline bool isUnifiedMailbox(const Collection &collection)
{
    return collection.resource() == QLatin1String("akonadi_unifiedmailbox_agent");
}

Q_REQUIRED_RESULT inline QString defaultIconName(const Collection &col)
{
    if (CollectionUtils::isVirtualParent(col)) {
        return QStringLiteral("edit-find");
    }
    if (col.isVirtual()) {
        return QStringLiteral("document-preview");
    }
    if (CollectionUtils::isResource(col)) {
        return QStringLiteral("network-server");
    }
    if (CollectionUtils::isStructural(col)) {
        return QStringLiteral("folder-grey");
    }
    if (CollectionUtils::isReadOnly(col)) {
        return QStringLiteral("folder-grey");
    }

    const QStringList content = col.contentMimeTypes();
    if ((content.size() == 1) || (content.size() == 2 && content.contains(Collection::mimeType()))) {
        if (content.contains(QLatin1String("text/x-vcard")) || content.contains(QLatin1String("text/directory"))
            || content.contains(QLatin1String("text/vcard"))) {
            return QStringLiteral("x-office-address-book");
        }
        // TODO: add all other content types and/or fix their mimetypes
        if (content.contains(QLatin1String("akonadi/event")) || content.contains(QLatin1String("text/ical"))) {
            return QStringLiteral("view-pim-calendar");
        }
        if (content.contains(QLatin1String("akonadi/task"))) {
            return QStringLiteral("view-pim-tasks");
        }
    } else if (content.isEmpty()) {
        return QStringLiteral("folder-grey");
    }
    return QStringLiteral("folder");
}
Q_REQUIRED_RESULT inline QString displayIconName(const Collection &col)
{
    QString iconName = defaultIconName(col);
    if (col.hasAttribute<EntityDisplayAttribute>() && !col.attribute<EntityDisplayAttribute>()->iconName().isEmpty()) {
        if (!col.attribute<EntityDisplayAttribute>()->activeIconName().isEmpty() && col.statistics().unreadCount() > 0) {
            iconName = col.attribute<EntityDisplayAttribute>()->activeIconName();
        } else {
            iconName = col.attribute<EntityDisplayAttribute>()->iconName();
        }
    }
    return iconName;
}
Q_REQUIRED_RESULT inline bool hasValidHierarchicalRID(const Collection &col)
{
    if (col == Collection::root()) {
        return true;
    }
    if (col.remoteId().isEmpty()) {
        return false;
    }
    return hasValidHierarchicalRID(col.parentCollection());
}
Q_REQUIRED_RESULT inline bool hasValidHierarchicalRID(const Item &item)
{
    return !item.remoteId().isEmpty() && hasValidHierarchicalRID(item.parentCollection());
}

/** Returns the collection represented by @p index.
 *  @param index has to be provided by an EntityTreeModel instance or a proxy model on top of one.
 */
Q_REQUIRED_RESULT AKONADICORE_EXPORT Collection fromIndex(const QModelIndex &index);
}

}
