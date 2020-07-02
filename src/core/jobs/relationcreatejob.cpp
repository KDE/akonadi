/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "relationcreatejob.h"
#include "job_p.h"
#include "relation.h"
#include "protocolhelper_p.h"
#include "private/protocol_p.h"
#include "akonadicore_debug.h"
#include <KLocalizedString>

using namespace Akonadi;

class Akonadi::RelationCreateJobPrivate : public JobPrivate
{
public:
    explicit RelationCreateJobPrivate(RelationCreateJob *parent)
        : JobPrivate(parent)
    {
    }

    Relation mRelation;
};

RelationCreateJob::RelationCreateJob(const Akonadi::Relation &relation, QObject *parent)
    : Job(new RelationCreateJobPrivate(this), parent)
{
    Q_D(RelationCreateJob);
    d->mRelation = relation;
}

void RelationCreateJob::doStart()
{
    Q_D(RelationCreateJob);

    if (!d->mRelation.isValid()) {
        qCWarning(AKONADICORE_LOG) << "The relation is invalid";
        setError(Job::Unknown);
        setErrorText(i18n("Failed to create relation."));
        emitResult();
        return;
    }

    d->sendCommand(Protocol::ModifyRelationCommandPtr::create(d->mRelation.left().id(),
                   d->mRelation.right().id(),
                   d->mRelation.type(),
                   d->mRelation.remoteId()));
}

bool RelationCreateJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    if (!response->isResponse() || response->type() != Protocol::Command::ModifyRelation) {
        return Job::doHandleResponse(tag, response);
    }

    return true;
}

Relation RelationCreateJob::relation() const
{
    Q_D(const RelationCreateJob);
    return d->mRelation;
}
