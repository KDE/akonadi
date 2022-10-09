/*
    SPDX-FileCopyrightText: 2019 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <intervalcheck.h>

#include <QSemaphore>

namespace Akonadi
{
namespace Server
{
class ItemRetrievalManager;

class FakeIntervalCheck : public IntervalCheck
{
    Q_OBJECT
public:
    explicit FakeIntervalCheck(ItemRetrievalManager &retrievalManager);
    void waitForInit();

protected:
    void init() override;

    bool shouldScheduleCollection(const Collection &) override;
    bool hasChanged(const Collection &collection, const Collection &changed) override;
    int collectionScheduleInterval(const Collection &collection) override;
    void collectionExpired(const Collection &collection) override;

private:
    QSemaphore m_initCalled;
};

} // namespace Server
} // namespace Akonadi
