/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>
#include <QStringList>

#include "agentinstancecreatejob.h"
#include "agentmanager.h"
#include "qtest_akonadi.h"

using namespace Akonadi;

class ResourceTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
    }
    void testResourceManagement()
    {
        qRegisterMetaType<Akonadi::AgentInstance>();
        QSignalSpy spyAddInstance(AgentManager::self(), &AgentManager::instanceAdded);
        QVERIFY(spyAddInstance.isValid());
        QSignalSpy spyRemoveInstance(AgentManager::self(), &AgentManager::instanceRemoved);
        QVERIFY(spyRemoveInstance.isValid());

        AgentType type = AgentManager::self()->type(QStringLiteral("akonadi_knut_resource"));
        QVERIFY(type.isValid());

        QStringList lst;
        lst << QStringLiteral("Resource");
        QCOMPARE(type.capabilities(), lst);

        auto job = new AgentInstanceCreateJob(type);
        AKVERIFYEXEC(job);

        AgentInstance instance = job->instance();
        QVERIFY(instance.isValid());

        QCOMPARE(spyAddInstance.count(), 1);
        QCOMPARE(spyAddInstance.first().at(0).value<AgentInstance>(), instance);
        QVERIFY(AgentManager::self()->instance(instance.identifier()).isValid());

        job = new AgentInstanceCreateJob(type);
        AKVERIFYEXEC(job);
        AgentInstance instance2 = job->instance();
        QVERIFY(!(instance == instance2));
        QCOMPARE(spyAddInstance.count(), 2);

        AgentManager::self()->removeInstance(instance);
        AgentManager::self()->removeInstance(instance2);
        QTRY_COMPARE(spyRemoveInstance.count(), 2);
        QVERIFY(!AgentManager::self()->instances().contains(instance));
        QVERIFY(!AgentManager::self()->instances().contains(instance2));
    }

    void testIllegalResourceManagement()
    {
        auto job = new AgentInstanceCreateJob(AgentManager::self()->type(QStringLiteral("non_existing_resource")));
        QVERIFY(!job->exec());

        // unique agent
        // According to vkrause the mailthreader agent is no longer started by
        // default so this won't work.
        /*
        const AgentType type = AgentManager::self()->type( "akonadi_mailthreader_agent" );
        QVERIFY( type.isValid() );
        job = new AgentInstanceCreateJob( type );
        AKVERIFYEXEC( job );

        job = new AgentInstanceCreateJob( type );
        QVERIFY( !job->exec() );
        */
    }
};

QTEST_AKONADIMAIN(ResourceTest)

#include "resourcetest.moc"
