/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "job.h"

namespace Akonadi
{
class ResourceSelectJobPrivate;

/**
 * @internal
 *
 * @short Job that selects a resource context for remote identifier based operations.
 *
 * This job selects a resource context that is used whenever remote identifier
 * based operations ( e.g. fetch items or collections by remote identifier ) are
 * executed.
 *
 * Example:
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * // Find out the akonadi id of the item with the remote id 'd1627013c6d5a2e7bb58c12560c27047'
 * // that is stored in the resource with identifier 'my_mail_resource'
 *
 * Session *m_resourceSession = new Session( "resourceSession" );
 *
 * ResourceSelectJob *job = new ResourceSelectJob( "my_mail_resource", resourceSession );
 *
 * connect( job, SIGNAL(result(KJob*)), SLOT(resourceSelected(KJob*)) );
 * ...
 *
 * void resourceSelected( KJob *job )
 * {
 *   if ( job->error() )
 *     return;
 *
 *   Item item;
 *   item.setRemoteIdentifier( "d1627013c6d5a2e7bb58c12560c27047" );
 *
 *   ItemFetchJob *fetchJob = new ItemFetchJob( item, m_resourceSession );
 *   connect( fetchJob, SIGNAL(result(KJob*)), SLOT(itemFetched(KJob*)) );
 * }
 *
 * void itemFetched( KJob *job )
 * {
 *   if ( job->error() )
 *     return;
 *
 *   const Item item = job->items().at(0);
 *
 *   qDebug() << "Remote id" << item.remoteId() << "has akonadi id" << item.id();
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT ResourceSelectJob : public Job
{
    Q_OBJECT
public:
    /**
     * Selects the specified resource for all following remote identifier
     * based operations in the same session.
     *
     * @param identifier The resource identifier, or any empty string to reset
     *                   the selection.
     * @param parent The parent object.
     */
    explicit ResourceSelectJob(const QString &identifier, QObject *parent = nullptr);

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    /// @cond PRIVATE
    Q_DECLARE_PRIVATE(ResourceSelectJob)
    /// @endcond
};

}
