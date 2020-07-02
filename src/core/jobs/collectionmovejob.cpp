/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionmovejob.h"
#include "changemediator_p.h"
#include "collection.h"
#include "job_p.h"
#include "protocolhelper_p.h"
#include "private/protocol_p.h"

#include <KLocalizedString>

using namespace Akonadi;

class Akonadi::CollectionMoveJobPrivate : public JobPrivate
{
public:
    explicit CollectionMoveJobPrivate(CollectionMoveJob *parent)
        : JobPrivate(parent)
    {
    }

    QString jobDebuggingString() const override;
    Collection destination;
    Collection collection;

    Q_DECLARE_PUBLIC(CollectionMoveJob)

};

QString Akonadi::CollectionMoveJobPrivate::jobDebuggingString() const
{
    return QStringLiteral("Move collection from %1 to %2").arg(collection.id()).arg(destination.id());
}

CollectionMoveJob::CollectionMoveJob(const Collection &collection, const Collection &destination, QObject *parent)
    : Job(new CollectionMoveJobPrivate(this), parent)
{
    Q_D(CollectionMoveJob);
    d->destination = destination;
    d->collection = collection;
}

void CollectionMoveJob::doStart()
{
    Q_D(CollectionMoveJob);

    if (!d->collection.isValid()) {
        setError(Job::Unknown);
        setErrorText(i18n("No objects specified for moving"));
        emitResult();
        return;
    }

    if (!d->destination.isValid() && d->destination.remoteId().isEmpty()) {
        setError(Job::Unknown);
        setErrorText(i18n("No valid destination specified"));
        emitResult();
        return;
    }

    const Scope colScope = ProtocolHelper::entitySetToScope(Collection::List() << d->collection);
    const Scope destScope = ProtocolHelper::entitySetToScope(Collection::List() << d->destination);

    d->sendCommand(Protocol::MoveCollectionCommandPtr::create(colScope, destScope));

    ChangeMediator::invalidateCollection(d->collection);
}

bool CollectionMoveJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    if (!response->isResponse() || response->type() != Protocol::Command::MoveCollection) {
        return Job::doHandleResponse(tag, response);
    }

    return true;
}
