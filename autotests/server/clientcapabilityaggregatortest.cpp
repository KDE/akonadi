/*
    Copyright (c) 2013 Volker Krause <vkrause@kde.org>

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

#include <QObject>
#include <QtTest/QTest>

#include "aktest.h"
#include "clientcapabilityaggregator.h"

using namespace Akonadi::Server;

class ClientCapabilityAggregatorTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testNotifyMessageVersion()
    {
        QCOMPARE(ClientCapabilityAggregator::minimumNotificationMessageVersion(), 0);
        QCOMPARE(ClientCapabilityAggregator::maximumNotificationMessageVersion(), 0);

        ClientCapabilities c;
        c.setNotificationMessageVersion(1);
        ClientCapabilityAggregator::addSession(c);
        QCOMPARE(ClientCapabilityAggregator::minimumNotificationMessageVersion(), 1);
        QCOMPARE(ClientCapabilityAggregator::maximumNotificationMessageVersion(), 1);

        ClientCapabilityAggregator::removeSession(c);
        QCOMPARE(ClientCapabilityAggregator::minimumNotificationMessageVersion(), 0);
        QCOMPARE(ClientCapabilityAggregator::maximumNotificationMessageVersion(), 0);

        ClientCapabilityAggregator::addSession(c);
        QCOMPARE(ClientCapabilityAggregator::minimumNotificationMessageVersion(), 1);
        QCOMPARE(ClientCapabilityAggregator::maximumNotificationMessageVersion(), 1);

        c.setNotificationMessageVersion(2);
        ClientCapabilityAggregator::addSession(c);
        QCOMPARE(ClientCapabilityAggregator::minimumNotificationMessageVersion(), 1);
        QCOMPARE(ClientCapabilityAggregator::maximumNotificationMessageVersion(), 2);
        ClientCapabilityAggregator::addSession(c);
        QCOMPARE(ClientCapabilityAggregator::minimumNotificationMessageVersion(), 1);
        QCOMPARE(ClientCapabilityAggregator::maximumNotificationMessageVersion(), 2);

        ClientCapabilityAggregator::removeSession(c);
        QCOMPARE(ClientCapabilityAggregator::minimumNotificationMessageVersion(), 1);
        QCOMPARE(ClientCapabilityAggregator::maximumNotificationMessageVersion(), 2);

        c.setNotificationMessageVersion(1);
        ClientCapabilityAggregator::removeSession(c);
        QCOMPARE(ClientCapabilityAggregator::minimumNotificationMessageVersion(), 2);
        QCOMPARE(ClientCapabilityAggregator::maximumNotificationMessageVersion(), 2);
    }
};

AKTEST_MAIN(ClientCapabilityAggregatorTest)

#include "clientcapabilityaggregatortest.moc"
