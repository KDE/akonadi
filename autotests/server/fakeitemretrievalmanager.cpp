/*
    SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "fakeitemretrievalmanager.h"
#include "storage/itemretrievalrequest.h"

using namespace Akonadi::Server;

Q_DECLARE_METATYPE(ItemRetrievalResult)

FakeItemRetrievalManager::FakeItemRetrievalManager()
{
    qRegisterMetaType<ItemRetrievalResult>();
}

void FakeItemRetrievalManager::requestItemDelivery(ItemRetrievalRequest request)
{
    QMetaObject::invokeMethod(this, [this, r = std::move(request)] {
            Q_EMIT requestFinished({r});
    }, Qt::QueuedConnection);
}

