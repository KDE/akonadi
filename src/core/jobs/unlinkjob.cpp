/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "unlinkjob.h"

#include "collection.h"
#include "job_p.h"
#include "linkjobimpl_p.h"

using namespace Akonadi;

class Akonadi::UnlinkJobPrivate : public LinkJobImpl<UnlinkJob>
{
public:
    explicit UnlinkJobPrivate(UnlinkJob *parent)
        : LinkJobImpl<UnlinkJob>(parent)
    {
    }
};

UnlinkJob::UnlinkJob(const Collection &collection, const Item::List &items, QObject *parent)
    : Job(new UnlinkJobPrivate(this), parent)
{
    Q_D(UnlinkJob);
    d->destination = collection;
    d->objectsToLink = items;
}

UnlinkJob::~UnlinkJob()
{
}

void UnlinkJob::doStart()
{
    Q_D(UnlinkJob);
    d->sendCommand(Protocol::LinkItemsCommand::Unlink);
}

bool UnlinkJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    Q_D(UnlinkJob);
    return d->handleResponse(tag, response);
}

#include "moc_unlinkjob.cpp"
