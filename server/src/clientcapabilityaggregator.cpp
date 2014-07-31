/***************************************************************************
 *   Copyright (C) 2013 by Volker Krause <vkrause@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "clientcapabilityaggregator.h"

#include <QVector>
#include <QMutex>

using namespace Akonadi::Server;

struct ClientCapabilityAggregatorData
{
    QMutex m_mutex;
    QVector<int> m_notifyVersions;
};

Q_GLOBAL_STATIC(ClientCapabilityAggregatorData, s_aggregator)

void ClientCapabilityAggregator::addSession(const ClientCapabilities &capabilities)
{
    if (capabilities.notificationMessageVersion() <= 0) {
        return;
    }
    QMutexLocker locker(&s_aggregator()->m_mutex);
    if (s_aggregator()->m_notifyVersions.size() <= capabilities.notificationMessageVersion()) {
        s_aggregator()->m_notifyVersions.resize(capabilities.notificationMessageVersion() + 1);
    }
    s_aggregator()->m_notifyVersions[capabilities.notificationMessageVersion()]++;
}

void ClientCapabilityAggregator::removeSession(const ClientCapabilities &capabilities)
{
    if (capabilities.notificationMessageVersion() <= 0) {
        return;
    }
    QMutexLocker locker(&s_aggregator()->m_mutex);
    s_aggregator()->m_notifyVersions[capabilities.notificationMessageVersion()]--;
    Q_ASSERT(s_aggregator()->m_notifyVersions.at(capabilities.notificationMessageVersion()) >= 0);
}

int ClientCapabilityAggregator::minimumNotificationMessageVersion()
{
    QMutexLocker locker(&s_aggregator()->m_mutex);
    for (int i = 1; i < s_aggregator()->m_notifyVersions.size(); ++i) {
        if (s_aggregator()->m_notifyVersions.at(i) != 0) {
            return i;
        }
    }
    return 0;
}

int ClientCapabilityAggregator::maximumNotificationMessageVersion()
{
    QMutexLocker locker(&s_aggregator()->m_mutex);
    for (int i = s_aggregator()->m_notifyVersions.size() - 1; i >= 1; --i) {
        if (s_aggregator()->m_notifyVersions.at(i) != 0) {
            return i;
        }
    }
    return 0;
}
