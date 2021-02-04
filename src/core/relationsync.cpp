/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
namespace Akonadi
{
class Item;
}

#include "relationsync.h"
#include "akonadicore_debug.h"
#include "itemfetchscope.h"

#include "jobs/itemfetchjob.h"
#include "jobs/relationcreatejob.h"
#include "jobs/relationdeletejob.h"
#include "jobs/relationfetchjob.h"

using namespace Akonadi;

RelationSync::RelationSync(QObject *parent)
    : Job(parent)
{
}

RelationSync::~RelationSync()
{
}

void RelationSync::setRemoteRelations(const Akonadi::Relation::List &relations)
{
    mRemoteRelations = relations;
    mRemoteRelationsSet = true;
    diffRelations();
}

void RelationSync::doStart()
{
    Akonadi::RelationFetchJob *fetch = new Akonadi::RelationFetchJob({Akonadi::Relation::GENERIC}, this);
    connect(fetch, &KJob::result, this, &RelationSync::onLocalFetchDone);
}

void RelationSync::onLocalFetchDone(KJob *job)
{
    auto fetch = static_cast<Akonadi::RelationFetchJob *>(job);
    mLocalRelations = fetch->relations();
    mLocalRelationsFetched = true;
    diffRelations();
}

void RelationSync::diffRelations()
{
    if (!mRemoteRelationsSet || !mLocalRelationsFetched) {
        qCDebug(AKONADICORE_LOG) << "waiting for delivery: " << mRemoteRelationsSet << mLocalRelationsFetched;
        return;
    }

    QHash<QByteArray, Akonadi::Relation> relationByRid;
    for (const Akonadi::Relation &localRelation : qAsConst(mLocalRelations)) {
        if (!localRelation.remoteId().isEmpty()) {
            relationByRid.insert(localRelation.remoteId(), localRelation);
        }
    }

    for (const Akonadi::Relation &remoteRelation : qAsConst(mRemoteRelations)) {
        if (relationByRid.contains(remoteRelation.remoteId())) {
            relationByRid.remove(remoteRelation.remoteId());
        } else {
            // New relation or had its GID updated, so create one now
            auto createJob = new RelationCreateJob(remoteRelation, this);
            connect(createJob, &KJob::result, this, &RelationSync::checkDone);
        }
    }

    for (const Akonadi::Relation &removedRelation : qAsConst(relationByRid)) {
        // Removed remotely, remove locally
        auto removeJob = new RelationDeleteJob(removedRelation, this);
        connect(removeJob, &KJob::result, this, &RelationSync::checkDone);
    }
    checkDone();
}

void RelationSync::slotResult(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << "Error during CollectionSync: " << job->errorString() << job->metaObject()->className();
        // pretend there were no errors
        Akonadi::Job::removeSubjob(job);
    } else {
        Akonadi::Job::slotResult(job);
    }
}

void RelationSync::checkDone()
{
    if (hasSubjobs()) {
        qCDebug(AKONADICORE_LOG) << "Still going";
        return;
    }
    qCDebug(AKONADICORE_LOG) << "done";
    emitResult();
}
