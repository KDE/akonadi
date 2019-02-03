/*
    Copyright (c) 2019 David Faure <faure@kde.org>

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

#include <aktest.h>

#include "aggregatedfetchscope.h"
#include "entities.h"
#include "notificationmanager.h"
#include "notificationsubscriber.h"

#include <QObject>
#include <QTest>

using namespace Akonadi;
using namespace Akonadi::Server;

class TestableNotificationSubscriber : public NotificationSubscriber
{
public:
    TestableNotificationSubscriber(NotificationManager *manager)
        : NotificationSubscriber(manager)
    {
        mSubscriber = "TestSubscriber";
    }

    using NotificationSubscriber::registerSubscriber;
    using NotificationSubscriber::modifySubscription;
    using NotificationSubscriber::disconnectSubscriber;

};

class NotificationManagerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testAggregatedFetchScope()
    {
        NotificationManager manager(AkThread::NoThread);
        QMetaObject::invokeMethod(&manager, "init", Qt::DirectConnection);

        // first subscriber, A
        TestableNotificationSubscriber subscriberA(&manager);
        Protocol::CreateSubscriptionCommand createCmd;
        createCmd.setSession("session1");
        subscriberA.registerSubscriber(createCmd);
        QVERIFY(!manager.tagFetchScope()->fetchIdOnly());
        QVERIFY(manager.tagFetchScope()->fetchAllAttributes()); // default is true
        QVERIFY(!manager.collectionFetchScope()->fetchIdOnly());
        QVERIFY(!manager.collectionFetchScope()->fetchStatistics());

        // set A's subscription settings
        Protocol::ModifySubscriptionCommand modifyCmd;
        {
            Protocol::TagFetchScope tagFetchScope;
            tagFetchScope.setFetchIdOnly(true);
            tagFetchScope.setFetchAllAttributes(false);
            modifyCmd.setTagFetchScope(tagFetchScope);

            Protocol::CollectionFetchScope collectionFetchScope;
            collectionFetchScope.setFetchIdOnly(true);
            collectionFetchScope.setIncludeStatistics(true);
            modifyCmd.setCollectionFetchScope(collectionFetchScope);

            Protocol::ItemFetchScope itemFetchScope;
            itemFetchScope.setFetch(Protocol::ItemFetchScope::FullPayload);
            itemFetchScope.setFetch(Protocol::ItemFetchScope::AllAttributes);
            itemFetchScope.setFetch(Protocol::ItemFetchScope::Size);
            itemFetchScope.setFetch(Protocol::ItemFetchScope::MTime);
            itemFetchScope.setFetch(Protocol::ItemFetchScope::RemoteRevision);
            itemFetchScope.setFetch(Protocol::ItemFetchScope::Flags);
            itemFetchScope.setFetch(Protocol::ItemFetchScope::RemoteID);
            itemFetchScope.setFetch(Protocol::ItemFetchScope::GID);
            itemFetchScope.setFetch(Protocol::ItemFetchScope::Tags);
            itemFetchScope.setFetch(Protocol::ItemFetchScope::Relations);
            itemFetchScope.setFetch(Protocol::ItemFetchScope::VirtReferences);
            modifyCmd.setItemFetchScope(itemFetchScope);
        }
        subscriberA.modifySubscription(modifyCmd);
        QVERIFY(manager.tagFetchScope()->fetchIdOnly());
        QVERIFY(!manager.tagFetchScope()->fetchAllAttributes());
        QVERIFY(manager.collectionFetchScope()->fetchIdOnly());
        QVERIFY(manager.collectionFetchScope()->fetchStatistics());
        QVERIFY(manager.itemFetchScope()->fullPayload());
        QVERIFY(manager.itemFetchScope()->allAttributes());

        // second subscriber, B
        TestableNotificationSubscriber subscriberB(&manager);
        subscriberB.registerSubscriber(createCmd);
        QVERIFY(!manager.tagFetchScope()->fetchIdOnly()); // A and B don't agree, so: false
        QVERIFY(manager.tagFetchScope()->fetchAllAttributes());
        QVERIFY(!manager.collectionFetchScope()->fetchIdOnly());
        QVERIFY(manager.collectionFetchScope()->fetchStatistics()); // at least one - so still true
        QVERIFY(manager.itemFetchScope()->fullPayload());
        QVERIFY(manager.itemFetchScope()->allAttributes());
        QVERIFY(manager.itemFetchScope()->fetchSize());
        QVERIFY(manager.itemFetchScope()->fetchMTime());
        QVERIFY(manager.itemFetchScope()->fetchRemoteRevision());
        QVERIFY(manager.itemFetchScope()->fetchFlags());
        QVERIFY(manager.itemFetchScope()->fetchRemoteId());
        QVERIFY(manager.itemFetchScope()->fetchGID());
        QVERIFY(manager.itemFetchScope()->fetchTags());
        QVERIFY(manager.itemFetchScope()->fetchRelations());
        QVERIFY(manager.itemFetchScope()->fetchVirtualReferences());

        // give it the same settings
        subscriberB.modifySubscription(modifyCmd);
        QVERIFY(manager.tagFetchScope()->fetchIdOnly()); // now they agree
        QVERIFY(!manager.tagFetchScope()->fetchAllAttributes());
        QVERIFY(manager.collectionFetchScope()->fetchIdOnly());
        QVERIFY(manager.collectionFetchScope()->fetchStatistics()); // no change for the "at least one" settings

        // revert B's settings, so we can check what happens when disconnecting
        modifyCmd.setTagFetchScope(Protocol::TagFetchScope());
        modifyCmd.setCollectionFetchScope(Protocol::CollectionFetchScope());
        subscriberB.modifySubscription(modifyCmd);
        QVERIFY(!manager.tagFetchScope()->fetchIdOnly());
        QVERIFY(manager.tagFetchScope()->fetchAllAttributes());
        QVERIFY(!manager.collectionFetchScope()->fetchIdOnly());
        QVERIFY(manager.collectionFetchScope()->fetchStatistics());

        // B goes away
        subscriberB.disconnectSubscriber();
        QVERIFY(manager.tagFetchScope()->fetchIdOnly()); // B cleaned up after itself, so A can have id-only again
        QVERIFY(!manager.tagFetchScope()->fetchAllAttributes());
        QVERIFY(manager.collectionFetchScope()->fetchIdOnly());
        QVERIFY(manager.collectionFetchScope()->fetchStatistics());

        // A goes away
        subscriberA.disconnectSubscriber();
        QVERIFY(!manager.collectionFetchScope()->fetchStatistics());
        QVERIFY(!manager.itemFetchScope()->fullPayload());
        QVERIFY(!manager.itemFetchScope()->allAttributes());
        QVERIFY(!manager.itemFetchScope()->fetchSize());
        QVERIFY(!manager.itemFetchScope()->fetchMTime());
        QVERIFY(!manager.itemFetchScope()->fetchRemoteRevision());
        QVERIFY(!manager.itemFetchScope()->fetchFlags());
        QVERIFY(!manager.itemFetchScope()->fetchRemoteId());
        QVERIFY(!manager.itemFetchScope()->fetchGID());
        QVERIFY(!manager.itemFetchScope()->fetchTags());
        QVERIFY(!manager.itemFetchScope()->fetchRelations());
        QVERIFY(!manager.itemFetchScope()->fetchVirtualReferences());
    }
};

AKTEST_MAIN(NotificationManagerTest)

#include "notificationmanagertest.moc"
