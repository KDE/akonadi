/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef RELATIONSYNC_H
#define RELATIONSYNC_H

#include "akonadicore_export.h"
#include "relation.h"

#include <KJob>

namespace Akonadi
{

class AKONADICORE_EXPORT RelationSync : public KJob
{
    Q_OBJECT
public:
    explicit RelationSync(QObject *parent = nullptr);
    ~RelationSync() override;

    void setRemoteRelations(const Akonadi::Relation::List &relations);

protected:
    void start() override;

private:
    void diffRelations();
    void checkDone();

private:
    Akonadi::Relation::List mRemoteRelations;
    Akonadi::Relation::List mLocalRelations;
    bool mRemoteRelationsSet = false;
    bool mLocalRelationsFetched = false;
    int mTasks = 0;
};

} // namespace Akonadi

#endif
