/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "relationdeletejob.h"
#include "job_p.h"
#include "relation.h"
#include "protocolhelper_p.h"
#include "private/protocol_p.h"
#include "akonadicore_debug.h"
#include <KLocalizedString>

using namespace Akonadi;

class Akonadi::RelationDeleteJobPrivate : public JobPrivate
{
public:
    explicit RelationDeleteJobPrivate(RelationDeleteJob *parent)
        : JobPrivate(parent)
    {
    }

    Relation mRelation;
};

RelationDeleteJob::RelationDeleteJob(const Akonadi::Relation &relation, QObject *parent)
    : Job(new RelationDeleteJobPrivate(this), parent)
{
    Q_D(RelationDeleteJob);
    d->mRelation = relation;
}

void RelationDeleteJob::doStart()
{
    Q_D(RelationDeleteJob);

    if (!d->mRelation.isValid()) {
        qCWarning(AKONADICORE_LOG) << "The relation is invalid";
        setError(Job::Unknown);
        setErrorText(i18n("Failed to create relation."));
        emitResult();
        return;
    }

    d->sendCommand(Protocol::RemoveRelationsCommandPtr::create(d->mRelation.left().id(),
                   d->mRelation.right().id(),
                   d->mRelation.type()));
}

bool RelationDeleteJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    if (!response->isResponse() || response->type() != Protocol::Command::RemoveRelations) {
        return Job::doHandleResponse(tag, response);
    }

    return true;
}

Relation RelationDeleteJob::relation() const
{
    Q_D(const RelationDeleteJob);
    return d->mRelation;
}
