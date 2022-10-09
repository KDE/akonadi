/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "akonadicore_export.h"

#include "jobs/job.h"
#include "relation.h"

namespace Akonadi
{
class AKONADICORE_EXPORT RelationSync : public Akonadi::Job
{
    Q_OBJECT
public:
    explicit RelationSync(QObject *parent = nullptr);
    ~RelationSync() override;

    void setRemoteRelations(const Akonadi::Relation::List &relations);

protected:
    void doStart() override;

private Q_SLOTS:
    void onLocalFetchDone(KJob *job);
    void slotResult(KJob *job) override;

private:
    void diffRelations();
    void checkDone();

private:
    Akonadi::Relation::List mRemoteRelations;
    Akonadi::Relation::List mLocalRelations;
    bool mRemoteRelationsSet = false;
    bool mLocalRelationsFetched = false;
};

}
