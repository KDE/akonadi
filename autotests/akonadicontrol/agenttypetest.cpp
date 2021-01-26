/*
 * SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
    googleContactsResource.mimeTypes = QStringList{QStringLiteral("text/directory"), QString()};
    googleContactsResource.capabilities = QStringList{AgentType::CapabilityResource};
    googleContactsResource.instanceCounter = 0;
    googleContactsResource.identifier = QStringLiteral("akonadi_googlecontacts_resource");
    googleContactsResource.custom =
        QVariantMap{{QStringLiteral("KAccounts"), QStringList{QStringLiteral("google-contacts"), QStringLiteral("google-calendar")}},
                    {QStringLiteral("HasLocalStorage"), true}};
    googleContactsResource.launchMethod = AgentType::Process;
    // We test an UTF-8 name within quotes.
    googleContactsResource.name = QStringLiteral("\"Контакти Google\"");
    // We also check whether an unquoted string with a comma is not parsed as a QStringList. See bug #330010
    googleContactsResource.comment = QStringLiteral("Доступ до ваших записів контактів, Google з KDE");
    googleContactsResource.icon = QStringLiteral("im-google");

    QTest::addColumn<QString>("fileName");
    QTest::addColumn<AgentType>("expectedAgentType");

    QTest::newRow("google contacts resource") << QFINDTESTDATA("data/akonaditestresource.desktop") << googleContactsResource;
}

void AgentTypeTest::testLoad()
{
    QFETCH(QString, fileName);
    QFETCH(AgentType, expectedAgentType);

    AgentType agentType;
    QLocale::setDefault(QLocale::Ukrainian);
    QVERIFY(agentType.load(fileName, nullptr));

    QCOMPARE(agentType.exec, expectedAgentType.exec);
    QCOMPARE(agentType.mimeTypes, expectedAgentType.mimeTypes);
    QCOMPARE(agentType.capabilities, expectedAgentType.capabilities);
    QCOMPARE(agentType.instanceCounter, expectedAgentType.instanceCounter);
    QCOMPARE(agentType.identifier, expectedAgentType.identifier);
    QCOMPARE(agentType.custom, expectedAgentType.custom);
    QCOMPARE(agentType.launchMethod, expectedAgentType.launchMethod);
    QCOMPARE(agentType.name, expectedAgentType.name);
    QCOMPARE(agentType.comment, expectedAgentType.comment);
    QCOMPARE(agentType.icon, expectedAgentType.icon);
}

AKTEST_MAIN(AgentTypeTest)

#include "agenttypetest.moc"
