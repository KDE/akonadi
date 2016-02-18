/*
 * Copyright (C) 2016  Elvis Angelaccio <elvis.angelaccio@kdemail.net>
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

#include <akonadicontrol/agenttype.h>
#include <shared/aktest.h>

#include <QTest>

Q_DECLARE_METATYPE(AgentType)

class AgentTypeTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void testLoad_data();
    void testLoad();
};

void AgentTypeTest::testLoad_data()
{
    AgentType googleContactsResource;
    googleContactsResource.exec = QStringLiteral("akonadi_googlecontacts_resource");
    googleContactsResource.mimeTypes = QStringList {QStringLiteral("text/directory"), QString()};
    googleContactsResource.capabilities = QStringList {AgentType::CapabilityResource};
    googleContactsResource.instanceCounter = 0;
    googleContactsResource.identifier = QStringLiteral("akonadi_googlecontacts_resource");
    googleContactsResource.custom = QVariantMap {
        {QStringLiteral("KAccounts"), QStringList {QStringLiteral("google-contacts"), QStringLiteral("google-calendar")}}
    };
    googleContactsResource.launchMethod = AgentType::Process;
    // We just test an UTF-8 entry
    googleContactsResource.name = {{QStringLiteral("uk"), QStringLiteral("Контакти Google")}};
    googleContactsResource.comment = {
        {QStringLiteral("uk"), QStringLiteral("Доступ до ваших записів контактів Google з KDE")},
        // Check for unquoted strings containing a comma. See bug #330010
        {QStringLiteral("string_with_comma"), QStringLiteral("This is just a QString, not a QStringList")},
        {QStringLiteral("quoted_string"), QStringLiteral("This is a QString")},
        {QStringLiteral("escaped_quote_string"), QStringLiteral("\"This is a QString\"")}
    };
    googleContactsResource.icon = QStringLiteral("im-google");


    QTest::addColumn<QString>("fileName");
    QTest::addColumn<AgentType>("expectedAgentType");

    QTest::newRow("google contacts resource") << QFINDTESTDATA("data/googlecontactsresource.desktop") << googleContactsResource;
}

void AgentTypeTest::testLoad()
{
    QFETCH(QString, fileName);
    QFETCH(AgentType, expectedAgentType);

    AgentType agentType;
    QVERIFY(agentType.load(fileName, Q_NULLPTR));

    QCOMPARE(agentType.exec, expectedAgentType.exec);
    QCOMPARE(agentType.mimeTypes, expectedAgentType.mimeTypes);
    QCOMPARE(agentType.capabilities, expectedAgentType.capabilities);
    QCOMPARE(agentType.instanceCounter, expectedAgentType.instanceCounter);
    QCOMPARE(agentType.identifier, expectedAgentType.identifier);
    QCOMPARE(agentType.custom, expectedAgentType.custom);
    QCOMPARE(agentType.launchMethod, expectedAgentType.launchMethod);
    QCOMPARE(agentType.name.value(QLatin1String("uk")), expectedAgentType.name.value(QLatin1String("uk")));
    QCOMPARE(agentType.comment.value(QLatin1String("uk")), expectedAgentType.comment.value(QLatin1String("uk")));
    QCOMPARE(agentType.comment.value(QLatin1String("string_with_comma")), expectedAgentType.comment.value(QLatin1String("string_with_comma")));
    QCOMPARE(agentType.comment.value(QLatin1String("quoted_string")), expectedAgentType.comment.value(QLatin1String("quoted_string")));
    QCOMPARE(agentType.comment.value(QLatin1String("escaped_quote_string")), expectedAgentType.comment.value(QLatin1String("escaped_quote_string")));
}

AKTEST_MAIN(AgentTypeTest)

#include "agenttypetest.moc"
