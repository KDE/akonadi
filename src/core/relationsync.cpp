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
#include "interface.h"

using namespace Akonadi;

RelationSync::RelationSync(QObject *parent)
    : KJob(parent)
{
}

RelationSync::~RelationSync() = default;

void RelationSync::setRemoteRelations(const Akonadi::Relation::List &relations)
{
    mRemoteRelations = relations;
    mRemoteRelationsSet = true;
    diffRelations();
}

void RelationSync::start()
{
    Akonadi::fetchRelationsOfTypes({Akonadi::Relation::GENERIC}).then(
        [this](const Relation::List &relations) {
            --mTasks;
            mLocalRelations = relations;
            mLocalRelationsFetched = true;
            diffRelations();
        },
        [this](const Akonadi::Error &error) {
            qCWarning(AKONADICORE_LOG) << "Error during RelationSync:" << error;
            --mTasks;
            checkDone();
        });
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
            //New relation or had its GID updated, so create one now
            Akonadi::createRelation(remoteRelation).then(
                [this](const Relation & /*relation*/) {
                    --mTasks;
                    checkDone();
                },
                [this](const Akonadi::Error &error) {
                    qCWarning(AKONADICORE_LOG) << "Error during RelationSync:" << error;
                    --mTasks;
                    checkDone();
                });
            ++mTasks;
        }
    }

    for (const Akonadi::Relation &removedRelation : qAsConst(relationByRid)) {
        //Removed remotely, remove locally
        Akonadi::deleteRelation(removedRelation).then(
            [this]() {
                --mTasks;
                checkDone();
            },
            [this](const Akonadi::Error &error) {
                qCWarning(AKONADICORE_LOG) << "Error during RelationSync:" << error;
                --mTasks;
                checkDone();
            });
        mTasks++;
    }
    checkDone();
}

void RelationSync::checkDone()
{
    if (mTasks > 0) {
        qCDebug(AKONADICORE_LOG) << "Still going";
        return;
    }
    qCDebug(AKONADICORE_LOG) << "done";
    emitResult();
}

