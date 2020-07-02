/*
    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "favoritecollectionattribute.h"

using namespace Akonadi;

Attribute *FavoriteCollectionAttribute::clone() const
{
    return new FavoriteCollectionAttribute();
}

QByteArray FavoriteCollectionAttribute::type() const
{
    return QByteArrayLiteral("favorite");
}

void FavoriteCollectionAttribute::deserialize(const QByteArray & /*data*/)
{
    // unused
}

QByteArray FavoriteCollectionAttribute::serialized() const
{
    return {};
}
