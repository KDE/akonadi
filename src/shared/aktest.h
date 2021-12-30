/*
    SPDX-FileCopyrightText: 2011 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akapplication.h"
#include <private/instance_p.h>

#include <KCrash>

#include <QTest>

#define AKTEST_MAIN(TestObject)                                                                                                                                \
    int main(int argc, char **argv)                                                                                                                            \
    {                                                                                                                                                          \
        qputenv("XDG_DATA_HOME", ".local-unit-test/share");                                                                                                    \
        qputenv("XDG_CONFIG_HOME", ".config-unit-test");                                                                                                       \
        AkCoreApplication app(argc, argv);                                                                                                                     \
        KCrash::setDrKonqiEnabled(false);                                                                                                                      \
        app.parseCommandLine();                                                                                                                                \
        TestObject tc;                                                                                                                                         \
        return QTest::qExec(&tc, argc, argv);                                                                                                                  \
    }

#define AKTEST_FAKESERVER_MAIN(TestObject)                                                                                                                     \
    int main(int argc, char **argv)                                                                                                                            \
    {                                                                                                                                                          \
        AkCoreApplication app(argc, argv);                                                                                                                     \
        KCrash::setDrKonqiEnabled(false);                                                                                                                      \
        app.addCommandLineOptions(QCommandLineOption(QStringLiteral("no-cleanup"), QStringLiteral("Don't clean up the temporary runtime environment")));       \
        app.parseCommandLine();                                                                                                                                \
        TestObject tc;                                                                                                                                         \
        return QTest::qExec(&tc, argc, argv);                                                                                                                  \
    }

#define AKCOMPARE(actual, expected)                                                                                                                            \
    do {                                                                                                                                                       \
        if (!QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__))                                                                        \
            return false;                                                                                                                                      \
    } while (false)

#define AKVERIFY(statement)                                                                                                                                    \
    do {                                                                                                                                                       \
        if (!QTest::qVerify((statement), #statement, "", __FILE__, __LINE__))                                                                                  \
            return false;                                                                                                                                      \
    } while (false)

inline void akTestSetInstanceIdentifier(const QString &instanceId)
{
    Akonadi::Instance::setIdentifier(instanceId);
}

#include <private/protocol_p.h>

namespace QTest
{
template<> char *toString(const Akonadi::Protocol::ItemChangeNotificationPtr &msg)
{
    return qstrdup(qPrintable(Akonadi::Protocol::debugString(msg)));
}
}

namespace AkTest
{
enum NtfField {
    NtfType = (1 << 0),
    NtfOperation = (1 << 1),
    NtfSession = (1 << 2),
    NtfEntities = (1 << 3),
    NtfResource = (1 << 5),
    NtfCollection = (1 << 6),
    NtfDestResource = (1 << 7),
    NtfDestCollection = (1 << 8),
    NtfAddedFlags = (1 << 9),
    NtfRemovedFlags = (1 << 10),
    NtfAddedTags = (1 << 11),
    NtfRemovedTags = (1 << 12),

    NtfFlags = NtfAddedFlags | NtfRemovedTags,
    NtfTags = NtfAddedTags | NtfRemovedTags,
    NtfAll = NtfType | NtfOperation | NtfSession | NtfEntities | NtfResource | NtfCollection | NtfDestResource | NtfDestCollection | NtfFlags | NtfTags
};
using NtfFields = QFlags<NtfField>;

bool compareNotifications(const Akonadi::Protocol::ItemChangeNotificationPtr &actual,
                          const Akonadi::Protocol::ItemChangeNotificationPtr &expected,
                          const NtfFields fields = NtfAll)
{
    if (fields & NtfOperation) {
        AKCOMPARE(actual->operation(), expected->operation());
    }
    if (fields & NtfSession) {
        AKCOMPARE(actual->sessionId(), expected->sessionId());
    }
    if (fields & NtfEntities) {
        AKCOMPARE(actual->items(), expected->items());
    }
    if (fields & NtfResource) {
        AKCOMPARE(actual->resource(), expected->resource());
    }
    if (fields & NtfCollection) {
        AKCOMPARE(actual->parentCollection(), expected->parentCollection());
    }
    if (fields & NtfDestResource) {
        AKCOMPARE(actual->destinationResource(), expected->destinationResource());
    }
    if (fields & NtfDestCollection) {
        AKCOMPARE(actual->parentDestCollection(), expected->parentDestCollection());
    }
    if (fields & NtfAddedFlags) {
        AKCOMPARE(actual->addedFlags(), expected->addedFlags());
    }
    if (fields & NtfRemovedFlags) {
        AKCOMPARE(actual->removedFlags(), expected->removedFlags());
    }
    if (fields & NtfAddedTags) {
        AKCOMPARE(actual->addedTags(), expected->addedTags());
    }
    if (fields & NtfRemovedTags) {
        AKCOMPARE(actual->removedTags(), expected->removedTags());
    }

    return true;
}
}

