/*
    Copyright (c) 2013 Sérgio Martins <iamsergio@gmail.com>

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

#include "itiphandlertest.h"
#include "helper.h"
#include "../mailclient_p.h"
#include "../fetchjobcalendar.h"

#include <kcalcore/attendee.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/qtest_akonadi.h>

#include <kcalcore/event.h>

#include <QString>
#include <QTestEventLoop>

using namespace Akonadi;
using namespace KCalCore;

Q_DECLARE_METATYPE(QList<Akonadi::IncidenceChanger::ChangeType>)
Q_DECLARE_METATYPE(Akonadi::ITIPHandler::Result)
Q_DECLARE_METATYPE(KCalCore::Attendee::PartStat)

static const char *s_ourEmail = "unittests@dev.nul"; // change also in kdepimlibs/akonadi/calendar/tests/unittestenv/kdehome/share/config


void ITIPHandlerTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();
    m_pendingItipMessageSignal = 0;
    MailClient::sRunningUnitTests = true;
}

void ITIPHandlerTest::testProcessITIPMessage_data()
{
    QTest::addColumn<QString>("data_filename");
    QTest::addColumn<QString>("action");
    QTest::addColumn<QString>("receiver");
    QTest::addColumn<QString>("incidenceUid"); // uid of incidence in invitation
    QTest::addColumn<bool>("testCancel"); // if true, after accepting, itip CANCEL is tested too
    QTest::addColumn<Akonadi::ITIPHandler::Result>("expectedResult");
    QTest::addColumn<int>("expectedNumEmails");
    QTest::addColumn<int>("expectedNumIncidences");
    QTest::addColumn<KCalCore::Attendee::PartStat>("expectedPartStat");

    QString data_filename;
    QString action = QLatin1String("accepted");
    QString incidenceUid = QString::fromLatin1("uosj936i6arrtl9c2i5r2mfuvg");
    QString receiver = QLatin1String(s_ourEmail);
    Akonadi::ITIPHandler::Result expectedResult;
    int expectedNumEmails = 0;
    int expectedNumIncidences = 0;
    KCalCore::Attendee::PartStat expectedPartStat;

    //----------------------------------------------------------------------------------------------
    // Someone invited us to an event, and we accept
    expectedResult = ITIPHandler::ResultSuccess;
    data_filename = QLatin1String("invited_us");
    expectedNumEmails = 0; // 0 e-mails are sent because the status update e-mail is sent by
                           // kmail's text_calendar.cpp.
    expectedNumIncidences = 1;
    expectedPartStat = KCalCore::Attendee::Accepted;
    action = QLatin1String("accepted");
    QTest::newRow("invited us1") << data_filename << action << receiver << incidenceUid << false
                                 << expectedResult
                                 << expectedNumEmails << expectedNumIncidences
                                 << expectedPartStat;
    //----------------------------------------------------------------------------------------------
    // Someone invited us to an event, and we accept conditionally
    expectedResult = ITIPHandler::ResultSuccess;
    data_filename = QLatin1String("invited_us");
    expectedNumEmails = 0; // 0 e-mails are sent because the status update e-mail is sent by
                           // kmail's text_calendar.cpp.
    expectedNumIncidences = 1;
    expectedPartStat = KCalCore::Attendee::Tentative;
    action = QLatin1String("tentative");
    QTest::newRow("invited us2") << data_filename << action << receiver << incidenceUid << false
                                 << expectedResult
                                 << expectedNumEmails << expectedNumIncidences
                                 << expectedPartStat;
    //----------------------------------------------------------------------------------------------
    // Someone invited us to an event, we delegate it
    expectedResult = ITIPHandler::ResultSuccess;
    data_filename = QLatin1String("invited_us");
    expectedNumEmails = 0; // 0 e-mails are sent because the status update e-mail is sent by
                           // kmail's text_calendar.cpp.

    // The e-mail to the delegate is sent by kmail's text_calendar.cpp
    expectedNumIncidences = 1;
    expectedPartStat = KCalCore::Attendee::Delegated;
    action = QLatin1String("delegated");
    QTest::newRow("invited us3") << data_filename << action << receiver << incidenceUid << false
                                 << expectedResult
                                 << expectedNumEmails << expectedNumIncidences
                                 << expectedPartStat;
    //----------------------------------------------------------------------------------------------
    // Process a CANCEL without having the incidence in our calendar.
    // itiphandler should return success and not error
    expectedResult = ITIPHandler::ResultSuccess;
    data_filename = QLatin1String("invited_us");
    expectedNumEmails = 0; // 0 e-mails are sent because the status update e-mail is sent by
                           // kmail's text_calendar.cpp.
    expectedNumIncidences = 0;
    action = QLatin1String("cancel");
    QTest::newRow("invited us4") << data_filename << action << receiver << incidenceUid << false
                                 << expectedResult
                                 << expectedNumEmails << expectedNumIncidences
                                 << expectedPartStat;
    //----------------------------------------------------------------------------------------------
    // Process a CANCEL for an incidence that we do have. It should be deleted.
    expectedResult = ITIPHandler::ResultSuccess;
    data_filename = QLatin1String("invited_us");
    expectedNumEmails = 0; // 0 e-mails are sent because the status update e-mail is sent by
                           // kmail's text_calendar.cpp.
    expectedNumIncidences = 1;
    action = QLatin1String("accepted");
    expectedPartStat = KCalCore::Attendee::Accepted;
    QTest::newRow("test cancel") << data_filename << action << receiver << incidenceUid
                                 << /**test_cancel = */ true
                                 << expectedResult
                                 << expectedNumEmails << expectedNumIncidences
                                 << expectedPartStat;

    //----------------------------------------------------------------------------------------------
    // Here we're testing an error case, where data is null.
    expectedResult = ITIPHandler::ResultError;
    expectedNumEmails = 0;
    expectedNumIncidences = 0;
    action = QLatin1String("accepted");
    QTest::newRow("invalid data") << QString() << action << receiver << incidenceUid << false
                                  << expectedResult
                                  << expectedNumEmails << expectedNumIncidences
                                  << expectedPartStat;
    //----------------------------------------------------------------------------------------------
    // Testing invalid action
    expectedResult = ITIPHandler::ResultError;
    data_filename = QLatin1String("invitation_us");
    expectedNumEmails = 0;
    expectedNumIncidences = 0;
    action = QLatin1String("accepted");
    QTest::newRow("invalid action") << data_filename << QString() << receiver << incidenceUid
                                    << false << expectedResult
                                    << expectedNumEmails << expectedNumIncidences
                                    << expectedPartStat;
    //----------------------------------------------------------------------------------------------
    // Test bug 235749
    expectedResult = ITIPHandler::ResultSuccess;
    data_filename = QLatin1String("bug235749");
    expectedNumEmails = 0;
    expectedNumIncidences = 1;
    expectedPartStat = KCalCore::Attendee::Accepted;
    action = QLatin1String("accepted");
    incidenceUid = QLatin1String("b6f0466a-8877-49d0-a4fc-8ee18ffd8e07"); // Don't change, hardcoded in data file
    QTest::newRow("bug 235749") << data_filename << action << receiver << incidenceUid << false
                                << expectedResult
                                << expectedNumEmails << expectedNumIncidences
                                << expectedPartStat;

    //----------------------------------------------------------------------------------------------
}

void ITIPHandlerTest::testProcessITIPMessage()
{
    QFETCH(QString, data_filename);
    QFETCH(QString, action);
    QFETCH(QString, receiver);
    QFETCH(QString, incidenceUid);
    QFETCH(bool, testCancel);
    QFETCH(Akonadi::ITIPHandler::Result, expectedResult);
    QFETCH(int, expectedNumEmails);
    QFETCH(int, expectedNumIncidences);
    QFETCH(KCalCore::Attendee::PartStat, expectedPartStat);

    MailClient::sUnitTestResults.clear();

    Akonadi::ITIPHandler *itipHandler = new Akonadi::ITIPHandler(this);
    itipHandler->setShowDialogsOnError(false);
    connect(itipHandler, SIGNAL(iTipMessageProcessed(Akonadi::ITIPHandler::Result,QString)),
            SLOT(oniTipMessageProcessed(Akonadi::ITIPHandler::Result,QString)) );

    m_expectedResult = expectedResult;

    data_filename = QLatin1String(ITIP_DATA_DIR) + QLatin1Char('/') + data_filename;

    QString iCalData = QString::fromLatin1(readFile(data_filename));

    m_pendingItipMessageSignal = 1;
    itipHandler->processiTIPMessage(receiver, iCalData, action);
    waitForIt();

    QCOMPARE(MailClient::sUnitTestResults.count(), expectedNumEmails);

    Item::List items = calendarItems();
    QCOMPARE(items.count(), expectedNumIncidences);

    if (expectedNumIncidences == 1) {
        KCalCore::Incidence::Ptr incidence = items.first().payload<KCalCore::Incidence::Ptr>();
        QVERIFY(incidence);
        QCOMPARE(incidence->schedulingID(), incidenceUid);
        QVERIFY(incidence->schedulingID() != incidence->uid());

        KCalCore::Attendee::List attendees = incidence->attendees();
        KCalCore::Attendee::Ptr me;
        foreach (const KCalCore::Attendee::Ptr &attendee, attendees) {
            if (attendee->email() == QLatin1String(s_ourEmail)) {
                me = attendee;
                break;
            }
        }
        QVERIFY(me);
        QCOMPARE(me->status(), expectedPartStat);
    }

    if (testCancel) {
        QVERIFY(!items.isEmpty());

        m_expectedResult = ITIPHandler::ResultSuccess;
        m_pendingItipMessageSignal = 1;
        itipHandler->processiTIPMessage(receiver, iCalData, QLatin1String("cancel"));
        waitForIt();

        QCOMPARE(MailClient::sUnitTestResults.count(), 0);

        // Check that they were deleted properly
        Item::List items = calendarItems();
        QVERIFY(items.isEmpty());
    }


    // Cleanup
    items = calendarItems();
    foreach (const Akonadi::Item &item, items) {
        ItemDeleteJob *job = new ItemDeleteJob(item);
        AKVERIFYEXEC(job);
    }

    delete itipHandler;
}

void ITIPHandlerTest::oniTipMessageProcessed(ITIPHandler::Result result, const QString &errorMessage)
{
    if (result != ITIPHandler::ResultSuccess && result != m_expectedResult) {
        qDebug() << "ITIPHandlerTest::oniTipMessageProcessed() error = " << errorMessage;
    }

    m_pendingItipMessageSignal--;
    QVERIFY(m_pendingItipMessageSignal >= 0);
    if (m_pendingItipMessageSignal == 0) {
        stopWaiting();
    }

    QCOMPARE(m_expectedResult, result);
}

QTEST_AKONADIMAIN(ITIPHandlerTest, GUI)
