/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "job.h"

namespace Akonadi
{
class Collection;
class CollectionCreateJobPrivate;

/**
 * @short Job that creates a new collection in the Akonadi storage.
 *
 * This job creates a new collection with all the set properties.
 * You have to use setParentCollection() to define the collection the
 * new collection shall be located in.
 *
 * @code
 *
 * // create a new top-level collection
 * Akonadi::Collection collection;
 * collection.setParentCollection( Collection::root() );
 * collection.setName( "Events" );
 * collection.setContentMimeTypes( QStringList( "text/calendar" ) );
 *
 * Akonadi::CollectionCreateJob *job = new Akonadi::CollectionCreateJob( collection );
 * connect( job, SIGNAL(result(KJob*)), this, SLOT(createResult(KJob*)) );
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT CollectionCreateJob : public Job
{
    Q_OBJECT
public:
    /**
     * Creates a new collection create job.
     *
     * @param collection The new collection. @p collection must have a parent collection
     * set with a unique identifier. If a resource context is specified in the current session
     * (that is you are using it within Akonadi::ResourceBase), the parent collection can be
     * identified by its remote identifier as well.
     * @param parent The parent object.
     */
    explicit CollectionCreateJob(const Collection &collection, QObject *parent = nullptr);

    /**
     * Destroys the collection create job.
     */
    ~CollectionCreateJob() override;

    /**
     * Returns the created collection if the job was executed successfully.
     */
    Q_REQUIRED_RESULT Collection collection() const;

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(CollectionCreateJob)
};

}

