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
class SubscriptionJobPrivate;

/*!
 * \internal
 *
 * \brief Job to manipulate the local subscription state of a set of collections.
 *
 * \class Akonadi::SubscriptionJob
 * \inheaderfile Akonadi/SubscriptionJob
 * \inmodule AkonadiCore
 */
class AKONADICORE_EXPORT SubscriptionJob : public Job
{
    Q_OBJECT
public:
    /*!
     * Creates a new subscription job.
     *
     * \a parent The parent object.
     */
    explicit SubscriptionJob(QObject *parent = nullptr);

    /*!
     * Destroys the subscription job.
     */
    ~SubscriptionJob() override;

    /*!
     * Subscribes to the given list of collections.
     *
     * \a collections List of collections to subscribe to.
     */
    void subscribe(const Collection::List &collections);

    /*!
     * Unsubscribes from the given list of collections.
     *
     * \a collections List of collections to unsubscribe from.
     */
    void unsubscribe(const Collection::List &collections);

protected:
    void doStart() override;
    void slotResult(KJob *job) override;

private:
    Q_DECLARE_PRIVATE(SubscriptionJob)
};

}
