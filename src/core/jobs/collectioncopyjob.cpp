/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectioncopyjob.h"
#include "collection.h"
#include "job_p.h"
#include "private/protocol_p.h"

#include <KLocalizedString>

using namespace Akonadi;

class Akonadi::CollectionCopyJobPrivate : public JobPrivate
{
public:
    explicit CollectionCopyJobPrivate(CollectionCopyJob *parent)
        : JobPrivate(parent)
    {
    }

    Collection mSource;
    Collection mTarget;

    QString jobDebuggingString() const override;
};

QString Akonadi::CollectionCopyJobPrivate::jobDebuggingString() const
{
    return QStringLiteral("copy collection from %1 to %2").arg(mSource.id()).arg(mTarget.id());
}

CollectionCopyJob::CollectionCopyJob(const Collection &source, const Collection &target, QObject *parent)
    : Job(new CollectionCopyJobPrivate(this), parent)
{
    Q_D(CollectionCopyJob);

    d->mSource = source;
    d->mTarget = target;
}

CollectionCopyJob::~CollectionCopyJob()
{
}

void CollectionCopyJob::doStart()
{
    Q_D(CollectionCopyJob);

    if (!d->mSource.isValid() && d->mSource.remoteId().isEmpty()) {
        setError(Unknown);
        setErrorText(i18n("Invalid collection to copy"));
        emitResult();
        return;
    }
    if (!d->mTarget.isValid() && d->mTarget.remoteId().isEmpty()) {
        setError(Unknown);
        setErrorText(i18n("Invalid destination collection"));
        emitResult();
        return;
    }
    d->sendCommand(Protocol::CopyCollectionCommandPtr::create(d->mSource.id(), d->mTarget.id()));
}

bool CollectionCopyJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    if (!response->isResponse() || response->type() != Protocol::Command::CopyCollection) {
        return Job::doHandleResponse(tag, response);
    }

    return true;
}

#include "moc_collectioncopyjob.cpp"
