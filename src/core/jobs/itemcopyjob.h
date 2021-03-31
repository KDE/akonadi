/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "item.h"
#include "job.h"

namespace Akonadi
{
class Collection;
class ItemCopyJobPrivate;

/**
 * @short Job that copies a set of items to a target collection in the Akonadi storage.
 *
 * The job can be used to copy one or several Item objects to another collection.
 *
 * Example:
 *
 * @code
 *
 * Akonadi::Item::List items = ...
 * Akonadi::Collection collection = ...
 *
 * Akonadi::ItemCopyJob *job = new Akonadi::ItemCopyJob( items, collection );
 * connect( job, SIGNAL(result(KJob*)), SLOT(jobFinished(KJob*)) );
 *
 * ...
 *
 * MyClass::jobFinished( KJob *job )
 * {
 *   if ( job->error() )
 *     qDebug() << "Error occurred";
 *   else
 *     qDebug() << "Items copied successfully";
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT ItemCopyJob : public Job
{
    Q_OBJECT

public:
    /**
     * Creates a new item copy job.
     *
     * @param item The item to copy.
     * @param target The target collection.
     * @param parent The parent object.
     */
    ItemCopyJob(const Item &item, const Collection &target, QObject *parent = nullptr);

    /**
     * Creates a new item copy job.
     *
     * @param items A list of items to copy.
     * @param target The target collection.
     * @param parent The parent object.
     */
    ItemCopyJob(const Item::List &items, const Collection &target, QObject *parent = nullptr);

    /**
     * Destroys the item copy job.
     */
    ~ItemCopyJob() override;

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(ItemCopyJob)
};

}

