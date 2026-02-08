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

/*!
 * \brief Job that creates a new collection in the Akonadi storage.
 *
 * This job creates a new collection with all the set properties.
 * You have to use setParentCollection() to define the collection the
 * new collection shall be located in.
 *
 * Example:
 *
 * \code
 * using namespace Qt::StringLiterals;
 *
 * // create a new top-level collection
 * Akonadi::Collection collection;
 * collection.setParentCollection(Collection::root());
 * collection.setName(u"Events"_s);
 * collection.setContentMimeTypes({ u""text/calendar"_s });
 *
 * auto job = new Akonadi::CollectionCreateJob(collection);
 * connect(job, &KJob::result, this, &MyClass::createResult);
 * \endcode
 *
 * \author Volker Krause <vkrause@kde.org>
 *
 * \class Akonadi::CollectionCreateJob
 * \inheaderfile Akonadi/CollectionCreateJob
 * \inmodule AkonadiCore
 */
class AKONADICORE_EXPORT CollectionCreateJob : public Job
{
    Q_OBJECT
public:
    /*!
     * Creates a new collection create job.
     *
     * \a collection The new collection. \a collection must have a parent collection
     * set with a unique identifier. If a resource context is specified in the current session
     * (that is you are using it within Akonadi::ResourceBase), the parent collection can be
     * identified by its remote identifier as well.
     * \a parent The parent object.
     */
    explicit CollectionCreateJob(const Collection &collection, QObject *parent = nullptr);

    /*!
     * Destroys the collection create job.
     */
    ~CollectionCreateJob() override;

    /*!
     * Returns the created collection if the job was executed successfully.
     */
    [[nodiscard]] Collection collection() const;

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(CollectionCreateJob)
};

}
