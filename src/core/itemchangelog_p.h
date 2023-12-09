/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 *
 */

#pragma once

#include "item.h"

#include "akonaditests_export.h"
#include "attributestorage_p.h"

namespace Akonadi
{
class AKONADI_TESTS_EXPORT ItemChangeLog
{
public:
    Q_REQUIRED_RESULT static ItemChangeLog *instance();

    Q_REQUIRED_RESULT Item::Flags &addedFlags(ItemPrivate *priv);
    Q_REQUIRED_RESULT Item::Flags addedFlags(const ItemPrivate *priv) const;
    Q_REQUIRED_RESULT Item::Flags &deletedFlags(ItemPrivate *priv);
    Q_REQUIRED_RESULT Item::Flags deletedFlags(const ItemPrivate *priv) const;

    Q_REQUIRED_RESULT Tag::List &addedTags(ItemPrivate *priv);
    Q_REQUIRED_RESULT Tag::List addedTags(const ItemPrivate *priv) const;
    Q_REQUIRED_RESULT Tag::List &deletedTags(ItemPrivate *priv);
    Q_REQUIRED_RESULT Tag::List deletedTags(const ItemPrivate *priv) const;

    Q_REQUIRED_RESULT AttributeStorage &attributeStorage(ItemPrivate *priv);
    Q_REQUIRED_RESULT AttributeStorage attributeStorage(const ItemPrivate *priv) const;

    void removeItem(const ItemPrivate *priv);
    void clearItemChangelog(const ItemPrivate *priv);

private:
    explicit ItemChangeLog();

    static ItemChangeLog *sInstance;

    QHash<ItemPrivate *, Item::Flags> m_addedFlags;
    QHash<ItemPrivate *, Item::Flags> m_deletedFlags;
    QHash<ItemPrivate *, Tag::List> m_addedTags;
    QHash<ItemPrivate *, Tag::List> m_deletedTags;
    QHash<ItemPrivate *, AttributeStorage> m_attributeStorage;
};

} // namespace Akonadi
