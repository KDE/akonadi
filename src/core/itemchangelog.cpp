/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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

Item::Flags &ItemChangeLog::addedFlags(const ItemPrivate *priv)
{
    return m_addedFlags[const_cast<ItemPrivate *>(priv)];
}

Item::Flags &ItemChangeLog::deletedFlags(const ItemPrivate *priv)
{
    return m_deletedFlags[const_cast<ItemPrivate *>(priv)];
}

Tag::List &ItemChangeLog::addedTags(const ItemPrivate *priv)
{
    return m_addedTags[const_cast<ItemPrivate *>(priv)];
}

Tag::List &ItemChangeLog::deletedTags(const ItemPrivate *priv)
{
    return m_deletedTags[const_cast<ItemPrivate *>(priv)];
}

AttributeStorage &ItemChangeLog::attributeStorage(ItemPrivate *priv)
{
    return m_attributeStorage[priv];
}

const AttributeStorage &ItemChangeLog::attributeStorage(const ItemPrivate *priv)
{
    return m_attributeStorage[const_cast<ItemPrivate *>(priv)];
}

void ItemChangeLog::removeItem(const ItemPrivate *priv)
{
    auto p = const_cast<ItemPrivate *>(priv);
    m_addedFlags.remove(p);
    m_deletedFlags.remove(p);
    m_addedTags.remove(p);
    m_deletedTags.remove(p);
    m_attributeStorage.remove(p);
}

void ItemChangeLog::clearItemChangelog(const ItemPrivate *priv)
{
    auto p = const_cast<ItemPrivate *>(priv);
    m_addedFlags.remove(p);
    m_deletedFlags.remove(p);
    m_addedTags.remove(p);
    m_deletedTags.remove(p);
    m_attributeStorage[p].resetChangeLog(); // keep the attributes
}
