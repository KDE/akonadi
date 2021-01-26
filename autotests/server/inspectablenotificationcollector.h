/*
 * SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef AKONADI_INSPECTABLENOTIFICATIONCOLLECTOR_H_
#define AKONADI_INSPECTABLENOTIFICATIONCOLLECTOR_H_

#include "storage/notificationcollector.h"

namespace Akonadi
{
namespace Server
{
class AkonadiServer;
class DataStore;

class InspectableNotificationCollector : public QObject, public NotificationCollector
{
    Q_OBJECT
public:
    InspectableNotificationCollector(AkonadiServer &akonadi, DataStore *store);
    ~InspectableNotificationCollector() override = default;

    void notify(Protocol::ChangeNotificationList &&ntfs) override;

Q_SIGNALS:
    void notifySignal(const Akonadi::Protocol::ChangeNotificationList &msgs);
};

} // namespace Server
} // namespace Akonadi

#endif
