/*
    SPDX-FileCopyrightText: 2008, 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_LINKJOBIMPL_P_H
#define AKONADI_LINKJOBIMPL_P_H

#include "collection.h"
#include "item.h"
#include "job.h"
#include "job_p.h"
#include "private/protocol_p.h"
#include "protocolhelper_p.h"

#include <KLocalizedString>

namespace Akonadi
{
/** Shared implementation details between item and collection move jobs. */
template<typename LinkJob> class LinkJobImpl : public JobPrivate
{
public:
    LinkJobImpl(Job *parent)
        : JobPrivate(parent)
    {
    }

    inline void sendCommand(Protocol::LinkItemsCommand::Action action)
    {
        auto q = static_cast<LinkJob *>(q_func()); // Job would be enough already, but then we don't have access to the non-public stuff...
        if (objectsToLink.isEmpty()) {
            q->emitResult();
            return;
        }
        if (!destination.isValid() && destination.remoteId().isEmpty()) {
            q->setError(Job::Unknown);
            q->setErrorText(i18n("No valid destination specified"));
            q->emitResult();
            return;
        }

        try {
            JobPrivate::sendCommand(
                Protocol::LinkItemsCommandPtr::create(action, ProtocolHelper::entitySetToScope(objectsToLink), ProtocolHelper::entityToScope(destination)));
        } catch (const std::exception &e) {
            q->setError(Job::Unknown);
            q->setErrorText(QString::fromUtf8(e.what()));
            q->emitResult();
            return;
        }
    }

    inline bool handleResponse(qint64 tag, const Protocol::CommandPtr &response)
    {
        auto q = static_cast<LinkJob *>(q_func());
        if (!response->isResponse() || response->type() != Protocol::Command::LinkItems) {
            return q->Job::doHandleResponse(tag, response);
        }

        return true;
    }

    Item::List objectsToLink;
    Collection destination;
};

}

#endif
