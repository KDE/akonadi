/*
    SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "storage/itemretrievalmanager.h"

namespace Akonadi
{
namespace Server
{
class FakeItemRetrievalManager : public ItemRetrievalManager
{
    Q_OBJECT
public:
    explicit FakeItemRetrievalManager();

    void requestItemDelivery(ItemRetrievalRequest request) override;
};

} // namespace Server
} // namespace Akonadi
