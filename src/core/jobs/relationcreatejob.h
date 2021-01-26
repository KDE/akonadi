/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_RELATIONCREATEJOB_H
#define AKONADI_RELATIONCREATEJOB_H

#include "job.h"

namespace Akonadi
{
class Relation;
class RelationCreateJobPrivate;

/**
 * @short Job that creates a new relation in the Akonadi storage.
 * @since 4.15
 */
class AKONADICORE_EXPORT RelationCreateJob : public Job
{
    Q_OBJECT

public:
    /**
     * Creates a new relation create job.
     *
     * @param relation The relation to create.
     * @param parent The parent object.
     */
    explicit RelationCreateJob(const Relation &relation, QObject *parent = nullptr);

    /**
     * Returns the relation.
     */
    Q_REQUIRED_RESULT Relation relation() const;

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(RelationCreateJob)
};

}

#endif
