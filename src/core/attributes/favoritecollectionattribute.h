/*
    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "attribute.h"

namespace Akonadi
{
class AKONADICORE_EXPORT FavoriteCollectionAttribute : public Attribute
{
public:
    explicit FavoriteCollectionAttribute() = default;

    Attribute *clone() const override;
    [[nodiscard]] QByteArray type() const override;

    void deserialize(const QByteArray &data) override;
    [[nodiscard]] QByteArray serialized() const override;
};

}
