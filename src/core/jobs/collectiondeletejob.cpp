/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectiondeletejob.h"
#include "collection.h"
#include "job_p.h"
#include "private/protocol_p.h"
#include "protocolhelper_p.h"

#include <KLocalizedString>

using namespace Akonadi;

class Akonadi::CollectionDeleteJobPrivate : public JobPrivate
{
public:
    explicit CollectionDeleteJobPrivate(CollectionDeleteJob *parent)
        : JobPrivate(parent)
    {
    }
    QString jobDebuggingString() const override;

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

#include "moc_collectiondeletejob.cpp"
