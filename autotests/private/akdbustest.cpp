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
#include <QTest>

#include <private/dbus_p.h>

#include <shared/aktest.h>

using namespace Akonadi;

Q_DECLARE_METATYPE(DBus::AgentType)

class DBusTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testServiceName()
    {
        akTestSetInstanceIdentifier(QString());
        QCOMPARE(DBus::serviceName(DBus::Server), QLatin1String("org.freedesktop.Akonadi"));
        akTestSetInstanceIdentifier(QLatin1String("foo"));
        QCOMPARE(DBus::serviceName(DBus::Server), QLatin1String("org.freedesktop.Akonadi.foo"));
    }

    void testParseAgentServiceName_data()
    {
        QTest::addColumn<QString>("instanceId");
        QTest::addColumn<QString>("serviceName");
        QTest::addColumn<QString>("agentId");
        QTest::addColumn<DBus::AgentType>("agentType");

        // generic invalid
        QTest::newRow("empty") << QString() << QString() << QString() << DBus::Unknown;
        QTest::newRow("wrong base") << QString() << "org.freedesktop.Agent.foo" << QString() << DBus::Unknown;
        QTest::newRow("wrong type") << QString() << "org.freedesktop.Akonadi.Randomizer.akonadi_maildir_resource_0" << QString() << DBus::Unknown;
        QTest::newRow("too long") << QString() << "org.freedesktop.Akonadi.Agent.akonadi_maildir_resource_0.foo.bar" << QString() << DBus::Unknown;

        // single instance cases
        QTest::newRow("agent, no multi-instance") << QString() << "org.freedesktop.Akonadi.Agent.akonadi_maildir_resource_0" << "akonadi_maildir_resource_0" << DBus::Agent;
        QTest::newRow("resource, no multi-instance") << QString() << "org.freedesktop.Akonadi.Resource.akonadi_maildir_resource_0" << "akonadi_maildir_resource_0" << DBus::Resource;
        QTest::newRow("preproc, no multi-instance") << QString() << "org.freedesktop.Akonadi.Preprocessor.akonadi_maildir_resource_0" << "akonadi_maildir_resource_0" << DBus::Preprocessor;
        QTest::newRow("multi-instance name in single-instance setup") << QString() <<  "org.freedesktop.Akonadi.Agent.akonadi_maildir_resource_0.foo" << QString() << DBus::Unknown;

        // multi-instance cases
        QTest::newRow("agent, multi-instance") << "foo" << "org.freedesktop.Akonadi.Agent.akonadi_maildir_resource_0.foo" << "akonadi_maildir_resource_0" << DBus::Agent;
        QTest::newRow("resource, multi-instance") << "foo" << "org.freedesktop.Akonadi.Resource.akonadi_maildir_resource_0.foo" << "akonadi_maildir_resource_0" << DBus::Resource;
        QTest::newRow("preproc, multi-instance") << "foo" << "org.freedesktop.Akonadi.Preprocessor.akonadi_maildir_resource_0.foo" << "akonadi_maildir_resource_0" << DBus::Preprocessor;
        QTest::newRow("single-instance name in multi-instance setup") << "foo" <<  "org.freedesktop.Akonadi.Agent.akonadi_maildir_resource_0" << QString() << DBus::Unknown;
    }

    void testParseAgentServiceName()
    {
        QFETCH(QString, instanceId);
        QFETCH(QString, serviceName);
        QFETCH(QString, agentId);
        QFETCH(DBus::AgentType, agentType);

        akTestSetInstanceIdentifier(instanceId);

        DBus::AgentType parsedType;
        QString parsedName = DBus::parseAgentServiceName(serviceName, parsedType);

        QCOMPARE(parsedName, agentId);
        QCOMPARE(parsedType, agentType);
    }

    void testAgentServiceName()
    {
        akTestSetInstanceIdentifier(QString());
        QCOMPARE(DBus::agentServiceName(QLatin1String("akonadi_maildir_resource_0"), DBus::Agent), QLatin1String("org.freedesktop.Akonadi.Agent.akonadi_maildir_resource_0"));

        akTestSetInstanceIdentifier(QLatin1String("foo"));
        QCOMPARE(DBus::agentServiceName(QLatin1String("akonadi_maildir_resource_0"), DBus::Agent), QLatin1String("org.freedesktop.Akonadi.Agent.akonadi_maildir_resource_0.foo"));
    }
};

AKTEST_MAIN(DBusTest)

#include "akdbustest.moc"
