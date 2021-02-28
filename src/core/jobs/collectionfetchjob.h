/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_COLLECTIONFETCHJOB_H
#define AKONADI_COLLECTIONFETCHJOB_H

#include "akonadicore_export.h"
#include "collection.h"
#include "job.h"

namespace Akonadi
{
class CollectionFetchScope;
class CollectionFetchJobPrivate;

/**
 * @short Job that fetches collections from the Akonadi storage.
 *
 * This class can be used to retrieve the complete or partial collection tree
 * from the Akonadi storage. This fetches collection data, not item data.
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * // fetching all collections containing emails recursively, starting at the root collection
 * CollectionFetchJob *job = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive, this);
 * job->fetchScope().setContentMimeTypes(QStringList() << "message/rfc822");
 * connect(job, SIGNAL(collectionsReceived(Akonadi::Collection::List)),
 *         this, SLOT(myCollectionsReceived(Akonadi::Collection::List)));
 * connect(job, SIGNAL(result(KJob*)), this, SLOT(collectionFetchResult(KJob*)));
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT CollectionFetchJob : public Job
{
    Q_OBJECT

public:
    /**
     * Describes the type of fetch depth.
     */
    enum Type {
        Base, ///< Only fetch the base collection.
        FirstLevel, ///< Only list direct sub-collections of the base collection.
        Recursive, ///< List all sub-collections.
        NonOverlappingRoots ///< List the roots of a list of fetched collections. @since 4.7
    };

    /**
     * Creates a new collection fetch job. If the given base collection
     * has a unique identifier, this is used to identify the collection in the
     * Akonadi server. If only a remote identifier is available the collection
     * is identified using that, provided that a resource search context has
     * been specified by calling setResource().
     *
     * @internal
     * For internal use only, if a remote identifier is set, the resource
     * search context can be set globally using ResourceSelectJob.
     * @endinternal
     *
     * @param collection The base collection for the listing.
     * @param type The type of fetch depth.
     * @param parent The parent object.
     */
    explicit CollectionFetchJob(const Collection &collection, Type type = FirstLevel, QObject *parent = nullptr);

    /**
     * Creates a new collection fetch job to retrieve a list of collections.
     * If a given collection has a unique identifier, this is used to identify
     * the collection in the Akonadi server. If only a remote identifier is
     * available the collection is identified using that, provided that a
     * resource search context has been specified by calling setResource().
     *
     * @internal
     * For internal use only, if a remote identifier is set, the resource
     * search context can be set globally using ResourceSelectJob.
     * @endinternal
     *
     * @param collections A list of collections to fetch. Must not be empty.
     * @param parent The parent object.
     */
    explicit CollectionFetchJob(const Collection::List &collections, QObject *parent = nullptr);

    /**
     * Creates a new collection fetch job to retrieve a list of collections.
     * If a given collection has a unique identifier, this is used to identify
     * the collection in the Akonadi server. If only a remote identifier is
     * available the collection is identified using that, provided that a
     * resource search context has been specified by calling setResource().
     *
     * @internal
     * For internal use only, if a remote identifier is set, the resource
     * search context can be set globally using ResourceSelectJob.
     * @endinternal
     *
     * @param collections A list of collections to fetch. Must not be empty.
     * @param type The type of fetch depth.
     * @param parent The parent object.
     * @todo KDE5 merge with ctor above.
     * @since 4.7
     */
    CollectionFetchJob(const Collection::List &collections, Type type, QObject *parent = nullptr);

    /**
     * Convenience ctor equivalent to CollectionFetchJob(const Collection::List &collections, Type type, QObject *parent = nullptr)
     * @since 4.8
     * @param collections list of collection ids
     * @param type fetch job type
     * @param parent parent object
     */
    explicit CollectionFetchJob(const QList<Collection::Id> &collections, Type type = Base, QObject *parent = nullptr);

    /**
     * Destroys the collection fetch job.
     */
    ~CollectionFetchJob() override;

    /**
     * Returns the list of fetched collection.
     */
    Q_REQUIRED_RESULT Collection::List collections() const;

    /**
     * Sets the collection fetch scope.
     *
     * The CollectionFetchScope controls how much of a collection's data is
     * fetched from the server as well as a filter to select which collections
     * to fetch.
     *
     * @param fetchScope The new scope for collection fetch operations.
     *
     * @see fetchScope()
     * @since 4.4
     */
    void setFetchScope(const CollectionFetchScope &fetchScope);

    /**
     * Returns the collection fetch scope.
     *
     * Since this returns a reference it can be used to conveniently modify the
     * current scope in-place, i.e. by calling a method on the returned reference
     * without storing it in a local variable. See the CollectionFetchScope documentation
     * for an example.
     *
     * @return a reference to the current collection fetch scope
     *
     * @see setFetchScope() for replacing the current collection fetch scope
     * @since 4.4
     */
    Q_REQUIRED_RESULT CollectionFetchScope &fetchScope();

Q_SIGNALS:
    /**
     * This signal is emitted whenever the job has received collections.
     *
     * @param collections The received collections.
     */
    void collectionsReceived(const Akonadi::Collection::List &collections);

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

protected Q_SLOTS:
    /// @cond PRIVATE
    void slotResult(KJob *job) override;
    /// @endcond

private:
    Q_DECLARE_PRIVATE(CollectionFetchJob)
};

}

#endif
