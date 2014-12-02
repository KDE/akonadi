/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTIONMODIFYJOB_H
#define AKONADI_COLLECTIONMODIFYJOB_H

#include "akonadicore_export.h"
#include "job.h"

namespace Akonadi {

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
 * MyAttribute *attribute = c.attribute<MyAttribute>( Entity::AddIfMissing );
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
    explicit CollectionModifyJob(const Collection &collection, QObject *parent = Q_NULLPTR);

    /**
     * Destroys the collection modify job.
     */
    ~CollectionModifyJob();

    /**
     * Returns the modified collection.
     *
     * @since 4.4
     */
    Collection collection() const;

protected:
    void doStart() Q_DECL_OVERRIDE;

private:
    Q_DECLARE_PRIVATE(CollectionModifyJob)
};

}

#endif
