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

#include "itemchangelog_p.h"

using namespace Akonadi;

ItemChangeLog *ItemChangeLog::sInstance = nullptr;

ItemChangeLog *ItemChangeLog::instance()
{
    if (!sInstance) {
        sInstance = new ItemChangeLog;
    }
    return sInstance;
}

ItemChangeLog::ItemChangeLog()
{
}

Item::Flags& ItemChangeLog::addedFlags(const ItemPrivate *priv)
{
    return m_addedFlags[const_cast<ItemPrivate *>(priv)];
}

Item::Flags& ItemChangeLog::deletedFlags(const ItemPrivate *priv)
{
    return m_deletedFlags[const_cast<ItemPrivate *>(priv)];
}

Tag::List& ItemChangeLog::addedTags(const ItemPrivate *priv)
{
    return m_addedTags[const_cast<ItemPrivate *>(priv)];
}

Tag::List& ItemChangeLog::deletedTags(const ItemPrivate *priv)
{
    return m_deletedTags[const_cast<ItemPrivate *>(priv)];
}

QSet<QByteArray>& ItemChangeLog::deletedAttributes(const ItemPrivate *priv)
{
    return m_deletedAttributes[const_cast<ItemPrivate *>(priv)];
}

void ItemChangeLog::clearItemChangelog(const ItemPrivate *priv)
{
    ItemPrivate *p = const_cast<ItemPrivate *>(priv);
    m_addedFlags.remove(p);
    m_deletedFlags.remove(p);
    m_addedTags.remove(p);
    m_deletedTags.remove(p);
    m_deletedAttributes.remove(p);
}
