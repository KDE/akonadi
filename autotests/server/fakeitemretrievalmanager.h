/*
    SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_SERVER_FAKEITEMRETRIEVALMANAGER_H
#define AKONADI_SERVER_FAKEITEMRETRIEVALMANAGER_H

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
#endif
