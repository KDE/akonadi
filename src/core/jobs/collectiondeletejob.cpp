/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "collectiondeletejob.h"
#include "collection.h"
#include "job_p.h"
#include "protocolhelper_p.h"
#include "private/protocol_p.h"

#include <KLocalizedString>

using namespace Akonadi;

class Akonadi::CollectionDeleteJobPrivate : public JobPrivate
{
public:
    CollectionDeleteJobPrivate(CollectionDeleteJob *parent)
        : JobPrivate(parent)
    {
    }
    QString jobDebuggingString() const Q_DECL_OVERRIDE;

    Collection mCollection;

};

QString Akonadi::CollectionDeleteJobPrivate::jobDebuggingString() const
{
    return QStringLiteral("Delete Collection id: %1").arg(mCollection.id());
}

CollectionDeleteJob::CollectionDeleteJob(const Collection &collection, QObject *parent)
    : Job(new CollectionDeleteJobPrivate(this), parent)
{
    Q_D(CollectionDeleteJob);

    d->mCollection = collection;
}

CollectionDeleteJob::~CollectionDeleteJob()
{
}

void CollectionDeleteJob::doStart()
{
    Q_D(CollectionDeleteJob);

    if (!d->mCollection.isValid() && d->mCollection.remoteId().isEmpty()) {
        setError(Unknown);
        setErrorText(i18n("Invalid collection"));
        emitResult();
        return;
    }

    d->sendCommand(Protocol::DeleteCollectionCommandPtr::create(ProtocolHelper::entityToScope(d->mCollection)));
}

bool CollectionDeleteJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    if (!response->isResponse() || response->type() != Protocol::Command::DeleteCollection) {
        return Job::doHandleResponse(tag, response);
    }

    return true;
}
