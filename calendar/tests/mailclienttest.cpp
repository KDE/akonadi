/*
    Copyright (c) 2011 Sérgio Martins <iamsergio@gmail.com>

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

// mailclient_p.cpp isn't exported so we include it directly.

#include "../mailclient_p.h"

#include <kcalcore/incidence.h>
#include <kcalcore/freebusy.h>
#include <mailtransport/messagequeuejob.h>
#include <kpimidentities/identity.h>

#include <akonadi/qtest_akonadi.h>

#include <QTestEventLoop>
#include <QtCore/QObject>

static const char *s_ourEmail = "unittests@dev.nul"; // change also in kdepimlibs/akonadi/calendar/tests/unittestenv/kdehome/share/config

using namespace Akonadi;

Q_DECLARE_METATYPE(KPIMIdentities::Identity)
Q_DECLARE_METATYPE(KCalCore::Incidence::Ptr)

class FakeMessageQueueJob : public MailTransport::MessageQueueJob
{
public:
    explicit FakeMessageQueueJob(QObject *parent = 0) : MailTransport::MessageQueueJob(parent)
    {
    }

    virtual void start()
    {
        UnitTestResult unitTestResult;
        unitTestResult.message     = message();
        unitTestResult.from        = addressAttribute().from();
        unitTestResult.to          = addressAttribute().to();
        unitTestResult.cc          = addressAttribute().cc();
        unitTestResult.bcc         = addressAttribute().bcc();
        unitTestResult.transportId = transportAttribute().transportId();
        FakeMessageQueueJob::sUnitTestResults << unitTestResult;

        setError(Akonadi::MailClient::ResultSuccess);
        setErrorText(QString());

        emitResult();
    }

    static UnitTestResult::List sUnitTestResults;
};

UnitTestResult::List FakeMessageQueueJob::sUnitTestResults;

class FakeITIPHandlerComponentFactory : public ITIPHandlerComponentFactory
{
public:
    explicit FakeITIPHandlerComponentFactory(QObject *parent = 0) : ITIPHandlerComponentFactory(parent)
    {
    }

    virtual MailTransport::MessageQueueJob *createMessageQueueJob(const KCalCore::IncidenceBase::Ptr &incidence, const KPIMIdentities::Identity &identity, QObject *parent = 0)
    {
        Q_UNUSED(incidence);
        Q_UNUSED(identity);
        return new FakeMessageQueueJob(parent);
    }
};

class MailClientTest : public QObject
{
    Q_OBJECT

private:
    MailClient *mMailClient;
    int mPendingSignals;
    MailClient::Result mLastResult;
    QString mLastErrorMessage;

private Q_SLOTS:

    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();

        mPendingSignals = 0;
        mMailClient = new MailClient(new FakeITIPHandlerComponentFactory(this), this);
        mLastResult = MailClient::ResultSuccess;
        connect(mMailClient, SIGNAL(finished(Akonadi::MailClient::Result,QString)),
                SLOT(handleFinished(Akonadi::MailClient::Result,QString)));
    }

    void cleanupTestCase()
    {
    }

    void testMailAttendees_data()
    {
        QTest::addColumn<KCalCore::Incidence::Ptr>("incidence");
        QTest::addColumn<KPIMIdentities::Identity>("identity");
        QTest::addColumn<bool>("bccMe");
        QTest::addColumn<QString>("attachment");
        QTest::addColumn<QString>("transport");
        QTest::addColumn<MailClient::Result>("expectedResult");
        QTest::addColumn<int>("expectedTransportId");
        QTest::addColumn<QString>("expectedFrom");
        QTest::addColumn<QStringList>("expectedToList");
        QTest::addColumn<QStringList>("expectedCcList");
        QTest::addColumn<QStringList>("expectedBccList");

        KCalCore::Incidence::Ptr incidence(new KCalCore::Event());
        KPIMIdentities::Identity identity;
        bool bccMe;
        QString attachment;
        QString transport;
        MailClient::Result expectedResult = MailClient::ResultNoAttendees;
        const int expectedTransportId = 69372773; // from tests/unittestenv/kdehome/share/config/mailtransports
        const QString expectedFrom = QLatin1String("unittests@dev.nul");   // from tests/unittestenv/kdehome/share/config/emailidentities
        KCalCore::Person::Ptr organizer(new KCalCore::Person(QLatin1String("Organizer"),
                                        QLatin1String("unittests@dev.nul")));

        QStringList toList;
        QStringList toCcList;
        QStringList toBccList;
        //----------------------------------------------------------------------------------------------
        QTest::newRow("No attendees") << incidence << identity << bccMe << attachment << transport
                                      << expectedResult << -1 << QString()
                                      << toList << toCcList << toBccList;
        //----------------------------------------------------------------------------------------------
        // One attendee, but without e-mail
        KCalCore::Attendee::Ptr attendee(new KCalCore::Attendee(QLatin1String("name1"),
                                         QString()));
        incidence = KCalCore::Incidence::Ptr(new KCalCore::Event());
        incidence->addAttendee(attendee);
        expectedResult = MailClient::ResultReallyNoAttendees;
        QTest::newRow("No attendees with email") << incidence << identity << bccMe << attachment << transport
                << expectedResult << -1 << QString()
                << toList << toCcList << toBccList;
        //----------------------------------------------------------------------------------------------
        // One valid attendee
        attendee = KCalCore::Attendee::Ptr(new KCalCore::Attendee(QLatin1String("name1"),
                                           QLatin1String("test@foo.org")));
        incidence = KCalCore::Incidence::Ptr(new KCalCore::Event());
        incidence->addAttendee(attendee);
        incidence->setOrganizer(organizer);
        expectedResult = MailClient::ResultSuccess;
        toList << QLatin1String("test@foo.org");
        QTest::newRow("One attendee") << incidence << identity << bccMe << attachment << transport
                                      << expectedResult << expectedTransportId << expectedFrom
                                      << toList << toCcList << toBccList;
        //----------------------------------------------------------------------------------------------
        // One valid attendee
        attendee = KCalCore::Attendee::Ptr(new KCalCore::Attendee(QLatin1String("name1"),
                                           QLatin1String("test@foo.org")));
        incidence = KCalCore::Incidence::Ptr(new KCalCore::Event());
        incidence->setOrganizer(organizer);
        incidence->addAttendee(attendee);
        QString invalidTransport = QLatin1String("foo");
        expectedResult = MailClient::ResultSuccess;
        // Should default to the default transport
        QTest::newRow("Invalid transport") << incidence << identity << bccMe << attachment
                                           << invalidTransport  << expectedResult
                                           << expectedTransportId << expectedFrom
                                           << toList << toCcList << toBccList;
        //----------------------------------------------------------------------------------------------
        // One valid attendee, and bcc me
        attendee = KCalCore::Attendee::Ptr(new KCalCore::Attendee(QLatin1String("name1"),
                                           QLatin1String("test@foo.org")));
        incidence = KCalCore::Incidence::Ptr(new KCalCore::Event());
        incidence->setOrganizer(organizer);
        incidence->addAttendee(attendee);
        expectedResult = MailClient::ResultSuccess;
        // Should default to the default transport
        toBccList.clear();
        toBccList << QLatin1String("unittests@dev.nul");
        QTest::newRow("Test bcc") << incidence << identity << /*bccMe*/true << attachment
                                  << transport  << expectedResult
                                  << expectedTransportId << expectedFrom
                                  << toList << toCcList << toBccList;
        //----------------------------------------------------------------------------------------------
        // Test CC list
        attendee = KCalCore::Attendee::Ptr(new KCalCore::Attendee(QLatin1String("name1"),
                                           QLatin1String("test@foo.org")));
        KCalCore::Attendee::Ptr optionalAttendee =
            KCalCore::Attendee::Ptr(new KCalCore::Attendee(QLatin1String("opt"),
                                    QLatin1String("optional@foo.org")));
        KCalCore::Attendee::Ptr nonParticipant =
            KCalCore::Attendee::Ptr(new KCalCore::Attendee(QLatin1String("non"),
                                    QLatin1String("non@foo.org")));
        optionalAttendee->setRole(KCalCore::Attendee::OptParticipant);
        nonParticipant->setRole(KCalCore::Attendee::NonParticipant);
        incidence = KCalCore::Incidence::Ptr(new KCalCore::Event());
        incidence->setOrganizer(organizer);
        incidence->addAttendee(attendee);
        incidence->addAttendee(optionalAttendee);
        incidence->addAttendee(nonParticipant);
        expectedResult = MailClient::ResultSuccess;
        // Should default to the default transport
        toBccList.clear();
        toBccList << QLatin1String("unittests@dev.nul");

        toCcList.clear();
        toCcList << QLatin1String("optional@foo.org")
                 << QLatin1String("non@foo.org");
        QTest::newRow("Test cc") << incidence << identity << /*bccMe*/true << attachment
                                 << transport  << expectedResult
                                 << expectedTransportId << expectedFrom
                                 << toList << toCcList << toBccList;
    }

    void testMailAttendees()
    {
        QFETCH(KCalCore::Incidence::Ptr, incidence);
        QFETCH(KPIMIdentities::Identity, identity);
        QFETCH(bool, bccMe);
        QFETCH(QString, attachment);
        QFETCH(QString, transport);
        QFETCH(MailClient::Result, expectedResult);
        QFETCH(int, expectedTransportId);
        QFETCH(QString, expectedFrom);
        QFETCH(QStringList, expectedToList);
        QFETCH(QStringList, expectedCcList);
        QFETCH(QStringList, expectedBccList);

        FakeMessageQueueJob::sUnitTestResults.clear();

        mPendingSignals = 1;
        mMailClient->mailAttendees(incidence, identity, bccMe, attachment, transport);
        waitForSignals();

        if (mLastResult != expectedResult) {
            qDebug() << "Fail1: last=" << mLastResult << "; expected=" << expectedResult
                     << "; error=" << mLastErrorMessage;
            QVERIFY(false);
        }

        UnitTestResult unitTestResult;
        if (FakeMessageQueueJob::sUnitTestResults.isEmpty()) {
            qDebug() << "mail results are empty";
        } else {
            unitTestResult = FakeMessageQueueJob::sUnitTestResults.first();
        }

        if (expectedTransportId != -1 && unitTestResult.transportId != expectedTransportId) {
            qDebug() << "got " << unitTestResult.transportId
                     << "; expected=" << expectedTransportId;
            QVERIFY(false);
        }

        QCOMPARE(unitTestResult.from, expectedFrom);
        QCOMPARE(unitTestResult.to, expectedToList);
        QCOMPARE(unitTestResult.cc, expectedCcList);
        QCOMPARE(unitTestResult.bcc, expectedBccList);
    }

    void testMailOrganizer_data()
    {
        QTest::addColumn<KCalCore::IncidenceBase::Ptr>("incidence");
        QTest::addColumn<KPIMIdentities::Identity>("identity");
        QTest::addColumn<QString>("from");
        QTest::addColumn<bool>("bccMe");
        QTest::addColumn<QString>("attachment");
        QTest::addColumn<QString>("subject");
        QTest::addColumn<QString>("transport");
        QTest::addColumn<MailClient::Result>("expectedResult");
        QTest::addColumn<int>("expectedTransportId");
        QTest::addColumn<QString>("expectedFrom");
        QTest::addColumn<QStringList>("expectedToList");
        QTest::addColumn<QStringList>("expectedBccList");
        QTest::addColumn<QString>("expectedSubject");

        KCalCore::IncidenceBase::Ptr incidence(new KCalCore::Event());
        KPIMIdentities::Identity identity;
        const QString from = QLatin1String(s_ourEmail);
        bool bccMe;
        QString attachment;
        QString subject = QLatin1String("subject1");
        QString transport;
        MailClient::Result expectedResult = MailClient::ResultSuccess;
        const int expectedTransportId = 69372773; // from tests/unittestenv/kdehome/share/config/mailtransports
        QString expectedFrom = from; // from tests/unittestenv/kdehome/share/config/emailidentities
        KCalCore::Person::Ptr organizer(new KCalCore::Person(QLatin1String("Organizer"),
                                        QLatin1String("unittests@dev.nul")));
        incidence->setOrganizer(organizer);

        QStringList toList;
        toList << QLatin1String("unittests@dev.nul");
        QStringList toBccList;
        QString expectedSubject;
        //----------------------------------------------------------------------------------------------
        expectedSubject = subject;
        QTest::newRow("test1") << incidence << identity << from << bccMe << attachment << subject
                               << transport << expectedResult << expectedTransportId << expectedFrom
                               << toList << toBccList << expectedSubject;
        //----------------------------------------------------------------------------------------------
        expectedSubject = QLatin1String("Free Busy Message");
        incidence = KCalCore::IncidenceBase::Ptr(new KCalCore::FreeBusy());
        incidence->setOrganizer(organizer);
        QTest::newRow("FreeBusy") << incidence << identity << from << bccMe << attachment << subject
                                  << transport << expectedResult << expectedTransportId << expectedFrom
                                  << toList << toBccList << expectedSubject;
    }

    void testMailOrganizer()
    {
        QFETCH(KCalCore::IncidenceBase::Ptr, incidence);
        QFETCH(KPIMIdentities::Identity, identity);
        QFETCH(QString, from);
        QFETCH(bool, bccMe);
        QFETCH(QString, attachment);
        QFETCH(QString, subject);
        QFETCH(QString, transport);
        QFETCH(MailClient::Result, expectedResult);
        QFETCH(int, expectedTransportId);
        QFETCH(QString, expectedFrom);
        QFETCH(QStringList, expectedToList);
        QFETCH(QStringList, expectedBccList);
        QFETCH(QString, expectedSubject);
        FakeMessageQueueJob::sUnitTestResults.clear();

        mPendingSignals = 1;
        mMailClient->mailOrganizer(incidence, identity, from, bccMe, attachment, subject, transport);
        waitForSignals();
        QCOMPARE(mLastResult, expectedResult);

        UnitTestResult unitTestResult = FakeMessageQueueJob::sUnitTestResults.first();
        if (expectedTransportId != -1)
            QCOMPARE(unitTestResult.transportId, expectedTransportId);

        QCOMPARE(unitTestResult.from, expectedFrom);
        QCOMPARE(unitTestResult.to, expectedToList);
        QCOMPARE(unitTestResult.bcc, expectedBccList);
        QCOMPARE(unitTestResult.message->subject()->asUnicodeString(), expectedSubject);
    }

    void testMailTo_data()
    {
        QTest::addColumn<KCalCore::IncidenceBase::Ptr>("incidence");
        QTest::addColumn<KPIMIdentities::Identity>("identity");
        QTest::addColumn<QString>("from");
        QTest::addColumn<bool>("bccMe");
        QTest::addColumn<QString>("recipients");
        QTest::addColumn<QString>("attachment");
        QTest::addColumn<QString>("transport");
        QTest::addColumn<MailClient::Result>("expectedResult");
        QTest::addColumn<int>("expectedTransportId");
        QTest::addColumn<QString>("expectedFrom");
        QTest::addColumn<QStringList>("expectedToList");
        QTest::addColumn<QStringList>("expectedBccList");

        KCalCore::IncidenceBase::Ptr incidence(new KCalCore::Event());
        KPIMIdentities::Identity identity;
        const QString from = QLatin1String(s_ourEmail);
        bool bccMe;
        const QString recipients = QLatin1String("unittests@dev.nul");
        QString attachment;
        QString transport;
        MailClient::Result expectedResult = MailClient::ResultSuccess;
        const int expectedTransportId = 69372773; // from tests/unittestenv/kdehome/share/config/mailtransports
        QString expectedFrom = from; // from tests/unittestenv/kdehome/share/config/emailidentities
        KCalCore::Person::Ptr organizer(new KCalCore::Person(QLatin1String("Organizer"),
                                        QLatin1String("unittests@dev.nul")));
        QStringList toList;
        toList << QLatin1String(s_ourEmail);
        QStringList toBccList;
        //----------------------------------------------------------------------------------------------
        QTest::newRow("test1") << incidence << identity << from << bccMe << recipients << attachment
                               << transport << expectedResult << expectedTransportId << expectedFrom
                               << toList << toBccList;
    }

    void testMailTo()
    {
        QFETCH(KCalCore::IncidenceBase::Ptr, incidence);
        QFETCH(KPIMIdentities::Identity, identity);
        QFETCH(QString, from);
        QFETCH(bool, bccMe);
        QFETCH(QString, recipients);
        QFETCH(QString, attachment);
        QFETCH(QString, transport);
        QFETCH(MailClient::Result, expectedResult);
        QFETCH(int, expectedTransportId);
        QFETCH(QString, expectedFrom);
        QFETCH(QStringList, expectedToList);
        QFETCH(QStringList, expectedBccList);
        FakeMessageQueueJob::sUnitTestResults.clear();

        mPendingSignals = 1;
        mMailClient->mailTo(incidence, identity, from, bccMe, recipients, attachment, transport);
        waitForSignals();
        QCOMPARE(mLastResult, expectedResult);
        UnitTestResult unitTestResult = FakeMessageQueueJob::sUnitTestResults.first();
        if (expectedTransportId != -1)
            QCOMPARE(unitTestResult.transportId, expectedTransportId);

        QCOMPARE(unitTestResult.from, expectedFrom);
        QCOMPARE(unitTestResult.to, expectedToList);
        QCOMPARE(unitTestResult.bcc, expectedBccList);
    }

    void handleFinished(Akonadi::MailClient::Result result, const QString &errorMessage)
    {
        kDebug() << "handleFinished: " << result << errorMessage;
        mLastResult = result;
        mLastErrorMessage = errorMessage;
        --mPendingSignals;
        QTestEventLoop::instance().exitLoop();
    }

    void waitForSignals()
    {
        if (mPendingSignals > 0) {
            QTestEventLoop::instance().enterLoop(5);   // 5 seconds is enough
            QVERIFY(!QTestEventLoop::instance().timeout());
        }
    }

public Q_SLOTS:
private:

};

QTEST_AKONADIMAIN(MailClientTest, GUI)

#include "mailclienttest.moc"
