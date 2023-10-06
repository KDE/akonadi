/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "collection.h"
#include "job.h"

namespace Akonadi
{
class Collection;
class SearchQuery;
class SearchCreateJobPrivate;

/**
 * @short Job that creates a virtual/search collection in the Akonadi storage.
 *
 * This job creates so called virtual or search collections, which don't contain
 * real data, but references to items that match a given search query.
 *
 * @code
 *
 * const QString name = "My search folder";
 * const QString query = "...";
 *
 * Akonadi::SearchCreateJob *job = new Akonadi::SearchCreateJob( name, query );
 * connect( job, SIGNAL(result(KJob*)), SLOT(jobFinished(KJob*)) );
 *
 * MyClass::jobFinished( KJob *job )
 * {
 *   if ( job->error() ) {
 *     qDebug() << "Error occurred";
 *     return;
 *   }
 *
 *   qDebug() << "Created search folder successfully";
 *   const Collection searchCollection = job->createdCollection();
 *   ...
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT SearchCreateJob : public Job
{
    Q_OBJECT

public:
    /**
     * Creates a search create job
     *
     * @param name The name of the search collection
     * @param searchQuery The search query
     * @param parent The parent object
     * @since 4.13
     */
    SearchCreateJob(const QString &name, const SearchQuery &searchQuery, QObject *parent = nullptr);

    /**
     * Sets list of mime types of items that search results can contain
     *
     * @param mimeTypes Mime types of items to include in search
     * @since 4.13
     */
    void setSearchMimeTypes(const QStringList &mimeTypes);

    /**
     * Returns list of mime types that search results can contain
     *
     * @since 4.13
     */
    [[nodiscard]] QStringList searchMimeTypes() const;

    /**
     * Sets list of collections to search in.
     *
     * When an empty list is set (default value), the search will contain
     * results from all collections that contain given mime types.
     *
     * @param collections Collections to search in, or an empty list to search all
     * @since 4.13
     */
    void setSearchCollections(const QList<Collection> &collections);

    /**
     * Returns list of collections to search in
     *
     * @since 4.13
     */
    [[nodiscard]] QList<Collection> searchCollections() const;

    /**
     * Sets whether resources should be queried too.
     *
     * When set to true, Akonadi will search local indexed items and will also
     * query resources that support server-side search, to forward the query
     * to remote storage (for example using SEARCH feature on IMAP servers) and
     * merge their results with results from local index.
     *
     * This is useful especially when searching resources, that don't fetch full
     * payload by default, for example the IMAP resource, which only fetches headers
     * by default and the body is fetched on demand, which means that emails that
     * were not yet fully fetched cannot be indexed in local index, and thus cannot
     * be searched. With remote search, even those emails can be included in search
     * results.
     *
     * This feature is disabled by default.
     *
     * @param enabled Whether remote search is enabled
     * @since 4.13
     */
    void setRemoteSearchEnabled(bool enabled);

    /**
     * Returns whether remote search is enabled.
     *
     * @since 4.13
     */
    [[nodiscard]] bool isRemoteSearchEnabled() const;

    /**
     * Sets whether the search should recurse into collections
     *
     * When set to true, all child collections of the specific collections will
     * be search recursively.
     *
     * @param recursive Whether to search recursively
     * @since 4.13
     */
    void setRecursive(bool recursive);

    /**
     * Returns whether the search is recursive
     *
     * @since 4.13
     */
    [[nodiscard]] bool isRecursive() const;

    /**
     * Destroys the search create job.
     */
    ~SearchCreateJob() override;

    /**
     * Returns the newly created search collection once the job finished successfully. Returns an invalid
     * collection if the job has not yet finished or failed.
     *
     * @since 4.4
     */
    [[nodiscard]] Collection createdCollection() const;

protected:
    /**
     * Reimplemented from Akonadi::Job
     */
    void doStart() override;

    /**
     * Reimplemented from Akonadi::Job
     */
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(SearchCreateJob)
};

}
