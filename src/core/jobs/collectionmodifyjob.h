/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "job.h"

namespace Akonadi
{
class CachePolicy;
class Collection;
class CollectionModifyJobPrivate;

/**
 * @short Job that modifies a collection in the Akonadi storage.
 *
 * This job modifies the properties of an existing collection.
 *
 * @code
 *
 * Akonadi::Collection collection = ...
 *
 * Akonadi::CollectionModifyJob *job = new Akonadi::CollectionModifyJob( collection );
 * connect( job, SIGNAL(result(KJob*)), this, SLOT(modifyResult(KJob*)) );
 *
 * @endcode
 *
 * If the collection has attributes, it is recommended only to supply values for
 * any attributes whose values are to be updated. This will help to avoid
 * potential clashes with other resources or applications which may happen to
 * update the collection simultaneously. To avoid supplying attribute values which
 * are not needed, create a new instance of the collection and explicitly set
 * attributes to be updated, e.g.
 *
 * @code
 *
 * // Update the 'MyAttribute' attribute of 'collection'.
 * Akonadi::Collection c( collection.id() );
 * MyAttribute *attribute = c.attribute<MyAttribute>( Collection::AddIfMissing );
 * if ( collection.hasAttribute<MyAttribute>() ) {
 *     *attribute = *collection.attribute<MyAttribute>();
 * }
 * // Update the value of 'attribute' ...
 * Akonadi::CollectionModifyJob *job = new Akonadi::CollectionModifyJob( c );
 * connect( job, SIGNAL(result(KJob*)), this, SLOT(modifyResult(KJob*)) );
 *
 * @endcode
 *
 * To update only the collection, and not change any attributes:
 *
 * @code
 *
 * // Update the cache policy for 'collection' to 'newPolicy'.
 * Akonadi::Collection c( collection.id() );
 * c.setCachePolicy( newPolicy );
 * Akonadi::CollectionModifyJob *job = new Akonadi::CollectionModifyJob( c );
 * connect( job, SIGNAL(result(KJob*)), this, SLOT(modifyResult(KJob*)) );
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT CollectionModifyJob : public Job
{
    Q_OBJECT

public:
    /**
     * Creates a new collection modify job for the given collection. The collection can be
     * identified either by its unique identifier or its remote identifier. Since the remote
     * identifier is not necessarily globally unique, identification by remote identifier only
     * works inside a resource context (that is from within ResourceBase) and is therefore
     * limited to one resource.
     *
     * @param collection The collection to modify.
     * @param parent The parent object.
     */
    explicit CollectionModifyJob(const Collection &collection, QObject *parent = nullptr);

    /**
     * Destroys the collection modify job.
     */
    ~CollectionModifyJob() override;

    /**
     * Returns the modified collection.
     *
     * @since 4.4
     */
    Q_REQUIRED_RESULT Collection collection() const;

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(CollectionModifyJob)
};

}
