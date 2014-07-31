/*
    Copyright (c) 2011 Volker Krause <vkrause@kde.org>

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

#include <akdbus.h>
#include <aktest.h>

Q_DECLARE_METATYPE(AkDBus::AgentType)

class AkDBusTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testServiceName()
    {
        akTestSetInstanceIdentifier(QString());
        QCOMPARE(AkDBus::serviceName(AkDBus::Server), QLatin1String("org.freedesktop.Akonadi"));
        akTestSetInstanceIdentifier(QLatin1String("foo"));
        QCOMPARE(AkDBus::serviceName(AkDBus::Server), QLatin1String("org.freedesktop.Akonadi.foo"));
    }

    void testParseAgentServiceName_data()
    {
        QTest::addColumn<QString>("instanceId");
        QTest::addColumn<QString>("serviceName");
        QTest::addColumn<QString>("agentId");
        QTest::addColumn<AkDBus::AgentType>("agentType");

        // generic invalid
        QTest::newRow("empty") << QString() << QString() << QString() << AkDBus::Unknown;
        QTest::newRow("wrong base") << QString() << "org.freedesktop.Agent.foo" << QString() << AkDBus::Unknown;
        QTest::newRow("wrong type") << QString() << "org.freedesktop.Akonadi.Randomizer.akonadi_maildir_resource_0" << QString() << AkDBus::Unknown;
        QTest::newRow("too long") << QString() << "org.freedesktop.Akonadi.Agent.akonadi_maildir_resource_0.foo.bar" << QString() << AkDBus::Unknown;

        // single instance cases
        QTest::newRow("agent, no multi-instance") << QString() << "org.freedesktop.Akonadi.Agent.akonadi_maildir_resource_0" << "akonadi_maildir_resource_0" << AkDBus::Agent;
        QTest::newRow("resource, no multi-instance") << QString() << "org.freedesktop.Akonadi.Resource.akonadi_maildir_resource_0" << "akonadi_maildir_resource_0" << AkDBus::Resource;
        QTest::newRow("preproc, no multi-instance") << QString() << "org.freedesktop.Akonadi.Preprocessor.akonadi_maildir_resource_0" << "akonadi_maildir_resource_0" << AkDBus::Preprocessor;
        QTest::newRow("multi-instance name in single-instance setup") << QString() <<  "org.freedesktop.Akonadi.Agent.akonadi_maildir_resource_0.foo" << QString() << AkDBus::Unknown;

        // multi-instance cases
        QTest::newRow("agent, multi-instance") << "foo" << "org.freedesktop.Akonadi.Agent.akonadi_maildir_resource_0.foo" << "akonadi_maildir_resource_0" << AkDBus::Agent;
        QTest::newRow("resource, multi-instance") << "foo" << "org.freedesktop.Akonadi.Resource.akonadi_maildir_resource_0.foo" << "akonadi_maildir_resource_0" << AkDBus::Resource;
        QTest::newRow("preproc, multi-instance") << "foo" << "org.freedesktop.Akonadi.Preprocessor.akonadi_maildir_resource_0.foo" << "akonadi_maildir_resource_0" << AkDBus::Preprocessor;
        QTest::newRow("single-instance name in multi-instance setup") << "foo" <<  "org.freedesktop.Akonadi.Agent.akonadi_maildir_resource_0" << QString() << AkDBus::Unknown;
    }

    void testParseAgentServiceName()
    {
        QFETCH(QString, instanceId);
        QFETCH(QString, serviceName);
        QFETCH(QString, agentId);
        QFETCH(AkDBus::AgentType, agentType);

        akTestSetInstanceIdentifier(instanceId);

        AkDBus::AgentType parsedType;
        QString parsedName = AkDBus::parseAgentServiceName(serviceName, parsedType);

        QCOMPARE(parsedName, agentId);
        QCOMPARE(parsedType, agentType);
    }

    void testAgentServiceName()
    {
        akTestSetInstanceIdentifier(QString());
        QCOMPARE(AkDBus::agentServiceName(QLatin1String("akonadi_maildir_resource_0"), AkDBus::Agent), QLatin1String("org.freedesktop.Akonadi.Agent.akonadi_maildir_resource_0"));

        akTestSetInstanceIdentifier(QLatin1String("foo"));
        QCOMPARE(AkDBus::agentServiceName(QLatin1String("akonadi_maildir_resource_0"), AkDBus::Agent), QLatin1String("org.freedesktop.Akonadi.Agent.akonadi_maildir_resource_0.foo"));
    }
};

AKTEST_MAIN(AkDBusTest)

#include "akdbustest.moc"
