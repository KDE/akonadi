/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "linkjob.h"

#include "collection.h"
#include "job_p.h"
#include "linkjobimpl_p.h"

using namespace Akonadi;

class Akonadi::LinkJobPrivate : public LinkJobImpl<LinkJob>
{
public:
    explicit LinkJobPrivate(LinkJob *parent)
        : LinkJobImpl<LinkJob>(parent)
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

LinkJob::~LinkJob()
{
}

void LinkJob::doStart()
{
    Q_D(LinkJob);
    d->sendCommand(Protocol::LinkItemsCommand::Link);
}

bool LinkJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    return d_func()->handleResponse(tag, response);
}

#include "moc_linkjob.cpp"
