/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "item.h"
#include "job.h"
#include "private/protocol_p.h"

namespace Akonadi
{
class Collection;
class UnlinkJobPrivate;

/*!
 * \brief Job that unlinks items inside the Akonadi storage.
 *
 * This job allows you to remove references to a set of items in a virtual
 * collection.
 *
 * Example:
 *
 * \code
 *
 * // Unlink the given items from the given collection
 * const Akonadi::Collection virtualCollection = ...
 * const Akonadi::Item::List items = ...
 *
 * Akonadi::UnlinkJob *job = new Akonadi::UnlinkJob( virtualCollection, items );
 * connect( job, SIGNAL(result(KJob*)), SLOT(jobFinished(KJob*)) );
 *
 * ...
 *
 * MyClass::jobFinished( KJob *job )
 * {
 *   if ( job->error() )
 *     qDebug() << "Error occurred";
 *   else
 *     qDebug() << "Unlinked items successfully";
 * }
 *
 * \endcode
 *
 * \class Akonadi::UnlinkJob
 * \inheaderfile Akonadi/UnlinkJob
 * \inmodule AkonadiCore
 *
 * \author Volker Krause <vkrause@kde.org>
 * \since 4.2
 * \sa LinkJob
 */
class AKONADICORE_EXPORT UnlinkJob : public Job
{
    Q_OBJECT
public:
    /*!
     * Creates a new unlink job.
     *
     * The job will remove references to the given items from the given collection.
     *
     * \a collection The collection from which the references should be removed.
     * \a items The items of which the references should be removed.
     * \a parent The parent object.
     */
    UnlinkJob(const Collection &collection, const Item::List &items, QObject *parent = nullptr);

    /*!
     * Destroys the unlink job.
     */
    ~UnlinkJob() override;

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(UnlinkJob)
    template<typename T, Protocol::LinkItemsCommand::Action>
    friend class LinkJobImpl;
};

}
