/*
    Copyright (c) 2011 Volker Krause <vkrause@kde.org>
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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

#ifndef AKTEST_H
#define AKTEST_H

#include "akapplication.h"
#include <QDebug>
#include <QBuffer>
#include <QTest>

#define AKTEST_MAIN(TestObject) \
int main(int argc, char **argv) \
{ \
    qputenv("XDG_DATA_HOME", ".local-unit-test/share"); \
    qputenv("XDG_CONFIG_HOME", ".config-unit-test"); \
    AkCoreApplication app(argc, argv); \
    app.parseCommandLine(); \
    TestObject tc; \
    return QTest::qExec(&tc, argc, argv); \
}

#define AKTEST_FAKESERVER_MAIN(TestObject) \
int main(int argc, char **argv) \
{ \
    FakeAkonadiServer::instance(); \
    AkCoreApplication app(argc, argv); \
    boost::program_options::options_description testOptions("Unit Test"); \
    testOptions.add_options() \
        ("no-cleanup", "Don't clean up the temporary runtime environment"); \
    app.addCommandLineOptions(testOptions); \
    app.parseCommandLine(); \
    /* HACK: Supernasty hack to remove AkCoreApplication options from argv before \
       it's passed to QTest, which will abort on custom-defined options */ \
    QList<char*> options; \
    for (int i = 0; i < argc; ++i) { \
        if (qstrcmp(argv[i], "--no-cleanup") > 0) { \
              options << argv[i]; \
        } \
    } \
    TestObject tc; \
    char **fakeArgv = (char **) malloc(options.count()); \
    for (int i = 0; i < options.count(); ++i) { \
        fakeArgv[i] = options[i]; \
    } \
    return QTest::qExec(&tc, options.count(), fakeArgv); \
}

// Takes from Qt 5
#if QT_VERSION < 0x050000

// Will try to wait for the expression to become true while allowing event processing
#define QTRY_VERIFY(__expr) \
do { \
    const int __step = 50; \
    const int __timeout = 5000; \
    if ( !( __expr ) ) { \
        QTest::qWait( 0 ); \
    } \
    for ( int __i = 0; __i < __timeout && !( __expr ); __i += __step ) { \
        QTest::qWait( __step ); \
    } \
    QVERIFY( __expr ); \
} while ( 0 )

// Will try to wait for the comparison to become successful while allowing event processing
#define QTRY_COMPARE(__expr, __expected) \
do { \
    const int __step = 50; \
    const int __timeout = 5000; \
    if ( ( __expr ) != ( __expected ) ) { \
        QTest::qWait( 0 ); \
    } \
    for ( int __i = 0; __i < __timeout && ( ( __expr ) != ( __expected ) ); __i += __step ) { \
        QTest::qWait( __step ); \
    } \
    QCOMPARE( __expr, __expected ); \
} while ( 0 )

#endif

#define AKCOMPARE(actual, expected) \
do {\
    if (!QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__))\
        return false;\
} while (0)

#define AKVERIFY(statement) \
do {\
    if (!QTest::qVerify((statement), #statement, "", __FILE__, __LINE__))\
        return false;\
} while (0)

inline void akTestSetInstanceIdentifier(const QString &instanceId)
{
    AkApplication::setInstanceIdentifier(instanceId);
}

#include <libs/notificationmessagev3_p.h>
#include <server/src/response.h>

namespace QTest {
template<>
char *toString(const Akonadi::NotificationMessageV3 &msg)
{
    QByteArray ba;
    QBuffer buf;
    buf.setBuffer(&ba);
    buf.open(QIODevice::WriteOnly);
    QDebug dbg(&buf);
    dbg.nospace() << msg;
    buf.close();
    return qstrdup(ba.constData());
}

template<>
char *toString(const Akonadi::Server::Response::ResultCode &response)
{
    switch (response) {
    case Akonadi::Server::Response::OK:
        return qstrdup("Response::OK");
    case Akonadi::Server::Response::BAD:
        return qstrdup("Response::BAD");
    case Akonadi::Server::Response::BYE:
        return qstrdup("Response::BYE");
    case Akonadi::Server::Response::NO:
        return qstrdup("Response::NO");
    case Akonadi::Server::Response::USER:
        return qstrdup("Response::USER");
    }
    Q_ASSERT(false);
    return nullptr;
}
}

namespace AkTest {
enum NtfField {
    NtfType                   = (1 << 0),
    NtfOperation              = (1 << 1),
    NtfSession                = (1 << 2),
    NtfEntities               = (1 << 3),
    NtfResource               = (1 << 5),
    NtfCollection             = (1 << 6),
    NtfDestResource           = (1 << 7),
    NtfDestCollection         = (1 << 8),
    NtfAddedFlags             = (1 << 9),
    NtfRemovedFlags           = (1 << 10),
    NtfAddedTags              = (1 << 11),
    NtfRemovedTags            = (1 << 12),

    NtfFlags                  = NtfAddedFlags | NtfRemovedTags,
    NtfTags                   = NtfAddedTags | NtfRemovedTags,
    NtfAll                    = NtfType | NtfOperation | NtfSession | NtfEntities |
                                NtfResource | NtfCollection | NtfDestResource |
                                NtfDestCollection | NtfFlags | NtfTags
};
typedef QFlags<NtfField> NtfFields;

bool compareNotifications(const Akonadi::NotificationMessageV3 &actual,
                          const Akonadi::NotificationMessageV3 &expected,
                          const NtfFields fields = NtfAll)
{
    if (fields & NtfType) {
        AKCOMPARE(actual.type(), expected.type());
    }
    if (fields & NtfOperation) {
        AKCOMPARE(actual.operation(), expected.operation());
    }
    if (fields & NtfSession) {
        AKCOMPARE(actual.sessionId(), expected.sessionId());
    }
    if (fields & NtfEntities) {
        AKCOMPARE(actual.entities(), expected.entities());
    }
    if (fields & NtfResource) {
        AKCOMPARE(actual.resource(), expected.resource());
    }
    if (fields & NtfCollection) {
        AKCOMPARE(actual.parentCollection(), expected.parentCollection());
    }
    if (fields & NtfDestResource) {
        AKCOMPARE(actual.destinationResource(), expected.destinationResource());
    }
    if (fields & NtfDestCollection) {
        AKCOMPARE(actual.parentDestCollection(), expected.parentDestCollection());
    }
    if (fields & NtfAddedFlags) {
        AKCOMPARE(actual.addedFlags(), expected.addedFlags());
    }
    if (fields & NtfRemovedFlags) {
        AKCOMPARE(actual.removedFlags(), expected.removedFlags());
    }
    if (fields & NtfAddedTags) {
        AKCOMPARE(actual.addedTags(), expected.addedTags());
    }
    if (fields & NtfRemovedTags) {
        AKCOMPARE(actual.removedTags(), expected.removedTags());
    }

    return true;
}
}

#endif
