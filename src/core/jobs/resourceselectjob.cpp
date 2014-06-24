/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "resourceselectjob_p.h"

#include <akonadi/private/imapparser_p.h>
#include "job_p.h"
#include <akonadi/private/protocol_p.h>


using namespace Akonadi;

class Akonadi::ResourceSelectJobPrivate : public JobPrivate
{
public:
    ResourceSelectJobPrivate(ResourceSelectJob *parent)
        : JobPrivate(parent)
    {
    }

    QString resourceId;
};

ResourceSelectJob::ResourceSelectJob(const QString &identifier, QObject *parent)
    : Job(new ResourceSelectJobPrivate(this), parent)
{
    Q_D(ResourceSelectJob);
    d->resourceId = identifier;
}

void ResourceSelectJob::doStart()
{
    Q_D(ResourceSelectJob);
    d->writeData(d->newTag() + " " AKONADI_CMD_RESOURCESELECT " " + ImapParser::quote(d->resourceId.toUtf8()) + '\n');
    emitWriteFinished();
}

#include "moc_resourceselectjob_p.cpp"
