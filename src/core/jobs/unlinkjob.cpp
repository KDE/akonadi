/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "unlinkjob.h"

#include "collection.h"
#include "job_p.h"
#include "linkjobimpl_p.h"

using namespace Akonadi;

class Akonadi::UnlinkJobPrivate : public LinkJobImpl<UnlinkJob>
{
public:
    UnlinkJobPrivate(UnlinkJob *parent)
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
