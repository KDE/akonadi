/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "job.h"
#include "relation.h"

namespace Akonadi
{
class Relation;
class RelationFetchJobPrivate;

/**
 * @short Job that to fetch relations from Akonadi storage.
 * @since 4.15
 */
class AKONADICORE_EXPORT RelationFetchJob : public Job
{
    Q_OBJECT

public:
    /**
     * Creates a new relation fetch job.
     *
     * @param relation The relation to fetch.
     * @param parent The parent object.
     */
    explicit RelationFetchJob(const Relation &relation, QObject *parent = nullptr);

    explicit RelationFetchJob(const QVector<QByteArray> &types, QObject *parent = nullptr);

    void setResource(const QString &identifier);

    /**
     * Returns the relations.
     */
    Q_REQUIRED_RESULT Relation::List relations() const;

Q_SIGNALS:
    /**
     * This signal is emitted whenever new relations have been fetched completely.
     *
     * @param relations The fetched relations.
     */
    void relationsReceived(const Akonadi::Relation::List &relations);

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(RelationFetchJob)
};

}
