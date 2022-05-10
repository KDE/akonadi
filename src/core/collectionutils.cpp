/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionutils.h"
#include "entitytreemodel.h"

using namespace Akonadi;

Collection CollectionUtils::fromIndex(const QModelIndex &index)
{
    return index.data(EntityTreeModel::CollectionRole).value<Collection>();
}
