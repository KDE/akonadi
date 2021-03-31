/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "job.h"
#include "tag.h"

namespace Akonadi
{
class TagFetchScope;
class TagFetchJobPrivate;

/**
 * @short Job that fetches tags from the Akonadi storage.
 *
 * This class is used to fetch tags from the Akonadi storage.
 *
 * If you want to fetch all items with given tag, use ItemFetchJob and the
 * ItemFetchJob(const Tag &tag, QObject *parent = nullptr) constructor (since 4.14)
 *
 * @since 4.13
 */
class AKONADICORE_EXPORT TagFetchJob : public Job
{
    Q_OBJECT

public:
    /**
     * Constructs a new tag fetch job that retrieves all tags stored in Akonadi.
     *
     * @param parent The parent object.
     */
    explicit TagFetchJob(QObject *parent = nullptr);

    /**
     * Constructs a new tag fetch job that retrieves the specified tag.
     * If the tag has a uid set, this is used to identify the tag on the Akonadi
     * server. If only a remote identifier is available, that is used. However
     * as remote identifiers are internal to resources, it's necessary to set
     * the resource context (see ResourceSelectJob).
     *
     * @param tag The tag to fetch.
     * @param parent The parent object.
     */
    explicit TagFetchJob(const Tag &tag, QObject *parent = nullptr);

    /**
     * Constructs a new tag fetch job that retrieves specified tags.
     * If the tags have a uid set, this is used to identify the tags on the Akonadi
     * server. If only a remote identifier is available, that is used. However
     * as remote identifiers are internal to resources, it's necessary to set
     * the resource context (see ResourceSelectJob).
     *
     * @param tags Tags to fetch.
     * @param parent The parent object.
     */
    explicit TagFetchJob(const Tag::List &tags, QObject *parent = nullptr);

    /**
     * Convenience ctor equivalent to ItemFetchJob(const Item::List &items, QObject *parent = nullptr)
     *
     * @param ids UIDs of tags to fetch.
     * @param parent The parent object.
     */
    explicit TagFetchJob(const QList<Tag::Id> &ids, QObject *parent = nullptr);

    /**
     * Sets the tag fetch scope.
     *
     * The TagFetchScope controls how much of an tags's data is fetched
     * from the server.
     *
     * @param fetchScope The new fetch scope for tag fetch operations.
     * @see fetchScope()
     */
    void setFetchScope(const TagFetchScope &fetchScope);

    /**
     * Returns the tag fetch scope.
     *
     * Since this returns a reference it can be used to conveniently modify the
     * current scope in-place, i.e. by calling a method on the returned reference
     * without storing it in a local variable. See the TagFetchScope documentation
     * for an example.
     *
     * @return a reference to the current tag fetch scope
     *
     * @see setFetchScope() for replacing the current tag fetch scope
     */
    TagFetchScope &fetchScope();

    /**
     * Returns the fetched tags after the job has been completed.
     */
    Q_REQUIRED_RESULT Tag::List tags() const;

Q_SIGNALS:
    /**
     * This signal is emitted whenever new tags have been fetched completely.
     *
     * @param tags The fetched tags
     */
    void tagsReceived(const Akonadi::Tag::List &tags);

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(TagFetchJob)

    /// @cond PRIVATE
    Q_PRIVATE_SLOT(d_func(), void timeout())
    /// @endcond
};

}

