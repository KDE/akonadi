/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "linkjob.h"

#include "collection.h"
#include "job_p.h"
#include "linkjobimpl_p.h"
#include "protocol_p.h"

using namespace Akonadi;

class Akonadi::LinkJobPrivate : public LinkJobImpl<LinkJob, Protocol::LinkItemsCommand::Link>
{
public:
    explicit LinkJobPrivate(LinkJob *parent)
        : LinkJobImpl(parent)
    {
    }
};

LinkJob::LinkJob(const Collection &collection, const Item::List &items, QObject *parent)
    : Job(new LinkJobPrivate(this), parent)
{
    Q_D(LinkJob);
    d->destination = collection;
    d->objectsToLink = items;
}

LinkJob::~LinkJob() = default;

void LinkJob::doStart()
{
    Q_D(LinkJob);
    d->doStart();
}

bool LinkJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    return d_func()->handleResponse(tag, response);
}

#include "moc_linkjob.cpp"
