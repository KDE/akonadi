/*
 * Copyright (C) 2018  Daniel Vrátil <dvratil@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "indexer_p.h"
#include "item.h"
#include "collection.h"
#include "typepluginloader_p.h"
#include "indexerinterface.h"

using namespace Akonadi;

QByteArray Indexer::index(const Item &item)
{
    if (auto plugin = qobject_cast<ItemIndexerInterface*>(
            TypePluginLoader::defaultObjectForMimeType(item.mimeType()))) {
        return plugin->index(item);
    }

    return {};
}

QByteArray Indexer::index(const Collection &collection)
{
    if (auto plugin = qobject_cast<CollectionIndexerInterface*>(
            TypePluginLoader::defaultObjectForMimeType(QStringLiteral("directory/node")))) {
        return plugin->index(collection);
    }

    return {};
}


