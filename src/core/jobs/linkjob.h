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
class LinkJobPrivate;

/**
 * @short Job that links items inside the Akonadi storage.
 *
 * This job allows you to create references to a set of items in a virtual
 * collection.
 *
 * Example:
 *
 * @code
 *
 * // Links the given items to the given virtual collection
 * const Akonadi::Collection virtualCollection = ...
 * const Akonadi::Item::List items = ...
 *
 * Akonadi::LinkJob *job = new Akonadi::LinkJob( virtualCollection, items );
 * connect( job, SIGNAL(result(KJob*)), SLOT(jobFinished(KJob*)) );
 *
 * ...
 *
 * MyClass::jobFinished( KJob *job )
 * {
 *   if ( job->error() )
 *     qDebug() << "Error occurred";
 *   else
 *     qDebug() << "Linked items successfully";
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 * @since 4.2
 * @see UnlinkJob
 */
class AKONADICORE_EXPORT LinkJob : public Job
{
    Q_OBJECT
public:
    /**
     * Creates the link job.
     *
     * The job will create references to the given items in the given collection.
     *
     * @param collection The collection in which the references should be created.
     * @param items The items of which the references should be created.
     * @param parent The parent object.
     */
    LinkJob(const Collection &collection, const Item::List &items, QObject *parent = nullptr);

    /**
     * Destroys the link job.
     */
    ~LinkJob() override;

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(LinkJob)
    template<typename T>
    friend class LinkJobImpl;
};

}
