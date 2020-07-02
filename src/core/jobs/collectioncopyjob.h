/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_COLLECTIONCOPYJOB_H
#define AKONADI_COLLECTIONCOPYJOB_H

#include "akonadicore_export.h"
#include "job.h"

namespace Akonadi
{

class Collection;
class CollectionCopyJobPrivate;

/**
 * @short Job that copies a collection into another collection in the Akonadi storage.
 *
 * This job copies a single collection into a specified target collection.
 *
 * @code
 *
 * Akonadi::Collection source = ...
 * Akonadi::Collection target = ...
 *
 * Akonadi::CollectionCopyJob *job = new Akonadi::CollectionCopyJob( source, target );
 * connect( job, SIGNAL(result(KJob*)), SLOT(copyFinished(KJob*)) );
 *
 * ...
 *
 * MyClass::copyFinished( KJob *job )
 * {
 *   if ( job->error() )
 *     qDebug() << "Error occurred";
 *   else
 *     qDebug() << "Copied successfully";
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT CollectionCopyJob : public Job
{
    Q_OBJECT

public:
    /**
     * Creates a new collection copy job to copy the given @p source collection into @p target.
     *
     * @param source The collection to copy.
     * @param target The target collection.
     * @param parent The parent object.
     */
    CollectionCopyJob(const Collection &source, const Collection &target, QObject *parent = nullptr);

    /**
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

#endif
