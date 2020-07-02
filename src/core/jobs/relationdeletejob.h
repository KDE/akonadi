/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_RELATIONDELETEJOB_H
#define AKONADI_RELATIONDELETEJOB_H

#include "job.h"

namespace Akonadi
{

class Relation;
class RelationDeleteJobPrivate;

/**
 * @short Job that deletes a relation in the Akonadi storage.
 * @since 4.15
 */
class AKONADICORE_EXPORT RelationDeleteJob : public Job
{
    Q_OBJECT

public:
    /**
     * Creates a new relation delete job.
     *
     * @param relation The relation to delete.
     * @param parent The parent object.
     */
    explicit RelationDeleteJob(const Relation &relation, QObject *parent = nullptr);

    /**
     * Returns the relation.
     */
    Q_REQUIRED_RESULT Relation relation() const;

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(RelationDeleteJob)
};

}

#endif
