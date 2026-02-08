/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "collection.h"
#include "job.h"

namespace Akonadi
{
class CollectionFetchScope;
class CollectionFetchJobPrivate;

/*!
 * \brief Job that fetches collections from the Akonadi storage.
 *
 * \class Akonadi::CollectionFetchJob
 * \inheaderfile Akonadi/CollectionFetchJob
 * \inmodule AkonadiCore
 *
 * This class can be used to retrieve the complete or partial collection tree
 * from the Akonadi storage. This fetches collection data, not item data.
 *
 * \code
 * using namespace Akonadi;
 * using namespace Qt::StringLiterals;
 *
 * // fetching all collections containing emails recursively, starting at the root collection
 * auto job = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive, this);
 * job->fetchScope().setContentMimeTypes({ u"message/rfc822"_s });
 * connect(job, &CollectionFetchJob::collectionsReceived, this &MyClass::myCollectionsReceived);
 * connect(job, &KJob::result, this, &MyClass::collectionFetchResult);
 *
 * \endcode
 *
 * \author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT CollectionFetchJob : public Job
{
    Q_OBJECT

public:
    /*!
     * Describes the type of fetch depth.
     */
    enum Type {
        Base, ///< Only fetch the base collection.
        FirstLevel, ///< Only list direct sub-collections of the base collection.
        Recursive, ///< List all sub-collections.
        NonOverlappingRoots ///< List the roots of a list of fetched collections. \since 4.7
    };

    /*!
     * Creates a new collection fetch job. If the given base collection
     * has a unique identifier, this is used to identify the collection in the
     * Akonadi server. If only a remote identifier is available the collection
     * is identified using that, provided that a resource search context has
     * been specified by calling setResource().
     *
     * \internal
     * For internal use only, if a remote identifier is set, the resource
     * search context can be set globally using ResourceSelectJob.
     * @endinternal
     *
     * \a collection The base collection for the listing.
     * \a type The type of fetch depth.
     * \a parent The parent object.
     */
    explicit CollectionFetchJob(const Collection &collection, Type type = FirstLevel, QObject *parent = nullptr);

    /*!
     * Creates a new collection fetch job to retrieve a list of collections.
     * If a given collection has a unique identifier, this is used to identify
     * the collection in the Akonadi server. If only a remote identifier is
     * available the collection is identified using that, provided that a
     * resource search context has been specified by calling setResource().
     *
     * \internal
     * For internal use only, if a remote identifier is set, the resource
     * search context can be set globally using ResourceSelectJob.
     * @endinternal
     *
     * \a collections A list of collections to fetch. Must not be empty.
     * \a parent The parent object.
     */
    explicit CollectionFetchJob(const Collection::List &collections, QObject *parent = nullptr);

    /*!
     * Creates a new collection fetch job to retrieve a list of collections.
     * If a given collection has a unique identifier, this is used to identify
     * the collection in the Akonadi server. If only a remote identifier is
     * available the collection is identified using that, provided that a
     * resource search context has been specified by calling setResource().
     *
     * \internal
     * For internal use only, if a remote identifier is set, the resource
     * search context can be set globally using ResourceSelectJob.
     * @endinternal
     *
     * \a collections A list of collections to fetch. Must not be empty.
     * \a type The type of fetch depth.
     * \a parent The parent object.
     * @todo KDE5 merge with ctor above.
     * \since 4.7
     */
    CollectionFetchJob(const Collection::List &collections, Type type, QObject *parent = nullptr);

    /*!
     * Convenience ctor equivalent to CollectionFetchJob(const Collection::List &collections, Type type, QObject *parent = nullptr)
     * \since 4.8
     * \a collections list of collection ids
     * \a type fetch job type
     * \a parent parent object
     */
    explicit CollectionFetchJob(const QList<Collection::Id> &collections, Type type = Base, QObject *parent = nullptr);

    /*!
     * Destroys the collection fetch job.
     */
    ~CollectionFetchJob() override;

    /*!
     * Returns the list of fetched collection.
     */
    [[nodiscard]] Collection::List collections() const;

    /*!
     * Sets the collection fetch scope.
     *
     * The CollectionFetchScope controls how much of a collection's data is
     * fetched from the server as well as a filter to select which collections
     * to fetch.
     *
     * \a fetchScope The new scope for collection fetch operations.
     *
     * \sa fetchScope()
     * \since 4.4
     */
    void setFetchScope(const CollectionFetchScope &fetchScope);

    /*!
     * Returns the collection fetch scope.
     *
     * Since this returns a reference it can be used to conveniently modify the
     * current scope in-place, i.e. by calling a method on the returned reference
     * without storing it in a local variable. See the CollectionFetchScope documentation
     * for an example.
     *
     * Returns a reference to the current collection fetch scope
     *
     * \sa setFetchScope() for replacing the current collection fetch scope
     * \since 4.4
     */
    [[nodiscard]] CollectionFetchScope &fetchScope();

Q_SIGNALS:
    /*!
     * This signal is emitted whenever the job has received collections.
     *
     * \a collections The received collections.
     */
    void collectionsReceived(const Akonadi::Collection::List &collections);

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

protected Q_SLOTS:
    void slotResult(KJob *job) override;

private:
    Q_DECLARE_PRIVATE(CollectionFetchJob)
};

}
