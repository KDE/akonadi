/*
 * Copyright (C) 2018  Daniel Vrátil <dvratil@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef AKONADI_INSPECTABLENOTIFICATIONCOLLECTOR_H_
#define AKONADI_INSPECTABLENOTIFICATIONCOLLECTOR_H_

#include "storage/notificationcollector.h"

namespace Akonadi {
namespace Server {

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
