/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef AKONADI_SUBSCRIPTIONJOB_P_H
#define AKONADI_SUBSCRIPTIONJOB_P_H

#include "akonadicore_export.h"
#include "collection.h"
#include "job.h"

namespace Akonadi {

class SubscriptionJobPrivate;

/**
 * @internal
 *
 * @short Job to manipulate the local subscription state of a set of collections.
 */
class AKONADICORE_EXPORT SubscriptionJob : public Job
{
    Q_OBJECT
public:
    /**
     * Creates a new subscription job.
     *
     * @param parent The parent object.
     */
    explicit SubscriptionJob(QObject *parent = 0);

    /**
     * Destroys the subscription job.
     */
    ~SubscriptionJob();

    /**
     * Subscribes to the given list of collections.
     *
     * @param collections List of collections to subscribe to.
     */
    void subscribe(const Collection::List &collections);

    /**
     * Unsubscribes from the given list of collections.
     *
     * @param collections List of collections to unsubscribe from.
     */
    void unsubscribe(const Collection::List &collections);

protected:
    void doStart() Q_DECL_OVERRIDE;
    void doHandleResponse(const QByteArray &tag, const QByteArray &data) Q_DECL_OVERRIDE;

private:
    Q_DECLARE_PRIVATE(SubscriptionJob)
};

}

#endif
