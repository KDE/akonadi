/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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
namespace Akonadi {
    class Item;
}

#include "relationsync.h"

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>

#include <akonadi/relationfetchjob.h>
#include <akonadi/relationcreatejob.h>
#include <akonadi/relationdeletejob.h>

#include <QStringList>

using namespace Akonadi;

RelationSync::RelationSync(QObject *parent)
    : Job(parent),
      mRemoteRelationsSet(false),
      mLocalRelationsFetched(false)
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
    QStringList types;
    types << QString::fromAscii(Akonadi::Relation::GENERIC);
    Akonadi::RelationFetchJob *fetch = new Akonadi::RelationFetchJob(types, this);
    connect(fetch, SIGNAL(result(KJob*)), this, SLOT(onLocalFetchDone(KJob*)));
}

void RelationSync::onLocalFetchDone(KJob *job)
{
    // kDebug();
    Akonadi::RelationFetchJob *fetch = static_cast<Akonadi::RelationFetchJob *>(job);
    mLocalRelations = fetch->relations();
    mLocalRelationsFetched = true;
    diffRelations();
}

void RelationSync::diffRelations()
{
    if (!mRemoteRelationsSet || !mLocalRelationsFetched) {
        kDebug() << "waiting for delivery: " << mRemoteRelationsSet << mLocalRelationsFetched;
        return;
    }

    // kDebug() << "diffing";
    QHash<QByteArray, Akonadi::Relation> relationByRid;
    Q_FOREACH (const Akonadi::Relation &localRelation, mLocalRelations) {
        if (!localRelation.remoteId().isEmpty()) {
            relationByRid.insert(localRelation.remoteId(), localRelation);
        }
    }

    Q_FOREACH (const Akonadi::Relation &remoteRelation, mRemoteRelations) {
        if (relationByRid.contains(remoteRelation.remoteId())) {
            relationByRid.remove(remoteRelation.remoteId());
        } else {
            //New relation or had its GID updated, so create one now
            RelationCreateJob *createJob = new RelationCreateJob(remoteRelation, this);
            connect(createJob, SIGNAL(result(KJob*)), this, SLOT(checkDone()));
        }
    }

    Q_FOREACH (const Akonadi::Relation &removedRelation, relationByRid) {
        //Removed remotely, remove locally
        RelationDeleteJob *removeJob = new RelationDeleteJob(removedRelation, this);
        connect(removeJob, SIGNAL(result(KJob*)), this, SLOT(checkDone()));
    }
    checkDone();
}

void RelationSync::slotResult(KJob *job)
{
    if (job->error()) {
        kWarning() << "Error during CollectionSync: " << job->errorString() << job->metaObject()->className();
        // pretend there were no errors
        Akonadi::Job::removeSubjob(job);
    } else {
        Akonadi::Job::slotResult(job);
    }
}

void RelationSync::checkDone()
{
    if (hasSubjobs()) {
        kDebug() << "Still going";
        return;
    }
    kDebug() << "done";
    emitResult();
}

#include "relationsync.moc"
