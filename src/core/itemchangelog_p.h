/*
 * Copyright 2015  Daniel Vrátil <dvratil@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef ITEMCHANGELOG_H
#define ITEMCHANGELOG_H

#include "item.h"

#include "akonadiprivate_export.h"

namespace Akonadi
{

class AKONADI_TESTS_EXPORT ItemChangeLog
{
public:
    static ItemChangeLog *instance();

    Item::Flags& addedFlags(const ItemPrivate *priv);
    Item::Flags& deletedFlags(const ItemPrivate *priv);

    Tag::List& addedTags(const ItemPrivate *priv);
    Tag::List& deletedTags(const ItemPrivate *priv);

    QSet<QByteArray>& deletedAttributes(const ItemPrivate *priv);

    void clearItemChangelog(const ItemPrivate *priv);

private:
    explicit ItemChangeLog();

    static ItemChangeLog *sInstance;

    QHash<ItemPrivate*, Item::Flags> m_addedFlags;
    QHash<ItemPrivate*, Item::Flags> m_deletedFlags;
    QHash<ItemPrivate*, Tag::List> m_addedTags;
    QHash<ItemPrivate*, Tag::List> m_deletedTags;
    QHash<ItemPrivate*, QSet<QByteArray>> m_deletedAttributes;
};

} // namespace Akonadi

#endif // ITEMCHANGELOG_H
