/*
    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_CORE_FAVORITECOLLECTIONATTRIBUTE_H_
#define AKONADI_CORE_FAVORITECOLLECTIONATTRIBUTE_H_

#include "attribute.h"
#include "akonadicore_export.h"

namespace Akonadi {

class AKONADICORE_EXPORT FavoriteCollectionAttribute : public Attribute
{
public:
    explicit FavoriteCollectionAttribute() = default;

    Attribute *clone() const override;
    QByteArray type() const override;

    void deserialize(const QByteArray &data) override;
    QByteArray serialized() const override;
};

}

#endif