/*
    SPDX-FileCopyrightText: 2008, 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "collection.h"
#include "item.h"
#include "job.h"
#include "job_p.h"
#include "private/protocol_p.h"
#include "protocolhelper_p.h"

#include <KLocalizedString>

#include <span>

namespace Akonadi
{
/** Shared implementation details between item and collection move jobs. */
template<typename LinkJob, Protocol::LinkItemsCommand::Action Action>
class LinkJobImpl : public JobPrivate
{
    static constexpr size_t MaxBatchSize = 10'000;

public:
    LinkJobImpl(Job *parent)
        : JobPrivate(parent)
    {
    }

    void doStart()
    {
        remainingObjects = std::span<Item>(objectsToLink);
        nextBatch();
    }

    bool nextBatch()
    {
        auto q = static_cast<LinkJob *>(q_func()); // Job would be enough already, but then we don't have access to the non-public stuff...
        if (!destination.isValid() && destination.remoteId().isEmpty()) {
            q->setError(Job::Unknown);
            q->setErrorText(i18n("No valid destination specified"));
            q->emitResult();
            return true;
        }

        if (remainingObjects.empty()) {
            return true;
        }

        const auto batchSize = std::min(MaxBatchSize, remainingObjects.size());
        if (batchSize == 0) {
            return true;
        }
        const auto batch = remainingObjects.subspan(0, batchSize);
        remainingObjects = remainingObjects.subspan(batchSize);
        const auto batchToLink = Item::List(batch.begin(), batch.end());

        try {
            JobPrivate::sendCommand(
                Protocol::LinkItemsCommandPtr::create(Action, ProtocolHelper::entitySetToScope(batchToLink), ProtocolHelper::entityToScope(destination)));
        } catch (const std::exception &e) {
            q->setError(Job::Unknown);
            q->setErrorText(QString::fromUtf8(e.what()));
            q->emitResult();
            return true;
        }

        return false;
    }

    inline bool handleResponse(qint64 tag, const Protocol::CommandPtr &response)
    {
        auto q = static_cast<LinkJob *>(q_func());
        if (!response->isResponse() || response->type() != Protocol::Command::LinkItems) {
            return q->Job::doHandleResponse(tag, response);
        }

        return nextBatch();
    }

    Item::List objectsToLink;
    std::span<const Item> remainingObjects;
    Collection destination;
};

}
