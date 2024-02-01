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
[[nodiscard]] inline bool isVirtualParent(const Collection &collection)
{
    return (collection.parentCollection() == Collection::root() && collection.isVirtual());
}

[[nodiscard]] inline bool isReadOnly(const Collection &collection)
{
    return !(collection.rights() & Collection::CanCreateItem);
}

[[nodiscard]] inline bool isRoot(const Collection &collection)
{
    return (collection == Collection::root());
}

[[nodiscard]] inline bool isResource(const Collection &collection)
{
    return (collection.parentCollection() == Collection::root());
}

[[nodiscard]] inline bool isStructural(const Collection &collection)
{
    return collection.contentMimeTypes().isEmpty();
}

[[nodiscard]] inline bool isFolder(const Collection &collection)
{
    return (!isRoot(collection) && !isResource(collection) && !isStructural(collection)
            && collection.resource() != QLatin1StringView("akonadi_search_resource"));
}

[[nodiscard]] inline bool isUnifiedMailbox(const Collection &collection)
{
    return collection.resource() == QLatin1StringView("akonadi_unifiedmailbox_agent");
}

[[nodiscard]] inline QString defaultIconName(const Collection &col)
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
        if (content.contains(QLatin1StringView("text/x-vcard")) || content.contains(QLatin1StringView("text/directory"))
            || content.contains(QLatin1StringView("text/vcard"))) {
            return QStringLiteral("x-office-address-book");
        }
        // TODO: add all other content types and/or fix their mimetypes
        if (content.contains(QLatin1StringView("akonadi/event")) || content.contains(QLatin1StringView("text/ical"))) {
            return QStringLiteral("view-pim-calendar");
        }
        if (content.contains(QLatin1StringView("akonadi/task"))) {
            return QStringLiteral("view-pim-tasks");
        }
    } else if (content.isEmpty()) {
        return QStringLiteral("folder-grey");
    }
    return QStringLiteral("folder");
}
[[nodiscard]] inline QString displayIconName(const Collection &col)
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
[[nodiscard]] inline bool hasValidHierarchicalRID(const Collection &col)
{
    if (col == Collection::root()) {
        return true;
    }
    if (col.remoteId().isEmpty()) {
        return false;
    }
    return hasValidHierarchicalRID(col.parentCollection());
}
[[nodiscard]] inline bool hasValidHierarchicalRID(const Item &item)
{
    return !item.remoteId().isEmpty() && hasValidHierarchicalRID(item.parentCollection());
}

/** Returns the collection represented by @p index.
 *  @param index has to be provided by an EntityTreeModel instance or a proxy model on top of one.
 */
[[nodiscard]] AKONADICORE_EXPORT Collection fromIndex(const QModelIndex &index);
}

}
