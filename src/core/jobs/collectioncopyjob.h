/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "job.h"

namespace Akonadi
{
class Collection;
class CollectionCopyJobPrivate;

/*!
 * \brief Job that copies a collection into another collection in the Akonadi storage.
 *
 * This job copies a single collection into a specified target collection.
 *
 * Example:
 *
 * \code
 *
 * Akonadi::Collection source = ...
 * Akonadi::Collection target = ...
 *
 * auto job = new Akonadi::CollectionCopyJob(source, target);
 * connect(job, &KJob::result, this, &MyClass::copyFinished);
 *
 * ...
 *
 * MyClass::copyFinished(KJob *job)
 * {
 *     if (job->error()) {
 *         qDebug() << "Error occurred";
 *     } else {
 *         qDebug() << "Copied successfully";
 *     }
 * }
 *
 * \endcode
 *
 * \class Akonadi::CollectionCopyJob
 * \inheaderfile Akonadi/CollectionCopyJob
 * \inmodule AkonadiCore
 *
 * \author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT CollectionCopyJob : public Job
{
    Q_OBJECT

public:
    /*!
     * Creates a new collection copy job to copy the given \a source collection into \a target.
     *
     * \a source The collection to copy.
     * \a target The target collection.
     * \a parent The parent object.
     */
    CollectionCopyJob(const Collection &source, const Collection &target, QObject *parent = nullptr);

    /*!
     * Destroys the collection copy job.
     */
    ~CollectionCopyJob() override;

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(CollectionCopyJob)
};

}
