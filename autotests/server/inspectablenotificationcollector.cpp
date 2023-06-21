/*
 * SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "inspectablenotificationcollector.h"

#include <QTest>

using namespace Akonadi;
using namespace Akonadi::Server;

InspectableNotificationCollector::InspectableNotificationCollector(AkonadiServer &akonadi, DataStore *store)
    : NotificationCollector(akonadi, store)
{
}

void InspectableNotificationCollector::notify(Protocol::ChangeNotificationList &&ntfs)
{
    Q_EMIT notifySignal(ntfs);
    NotificationCollector::notify(std::move(ntfs));
}

#include "moc_inspectablenotificationcollector.cpp"
