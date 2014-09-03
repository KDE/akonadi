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
#include "../utils_p.h"

#include <kcalcore/icalformat.h>
#include <kcalcore/attendee.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/qtest_akonadi.h>
#include <mailtransport/messagequeuejob.h>

#include <kcalcore/event.h>

#include <QString>

using namespace Akonadi;
using namespace KCalCore;

Q_DECLARE_METATYPE(Akonadi::IncidenceChanger::InvitationPolicy)
Q_DECLARE_METATYPE(QList<Akonadi::IncidenceChanger::ChangeType>)
Q_DECLARE_METATYPE(Akonadi::ITIPHandler::Result)
Q_DECLARE_METATYPE(KCalCore::Attendee::PartStat)
Q_DECLARE_METATYPE(QList<int>)

static const char *s_ourEmail = "unittests@dev.nul"; // change also in kdepimlibs/akonadi/calendar/tests/unittestenv/kdehome/share/config
static const char *s_outEmail2 = "identity2@kde.org";

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
    FakeITIPHandlerComponentFactory(QObject *parent = 0) : ITIPHandlerComponentFactory(parent)
    {
    }

    virtual MailTransport::MessageQueueJob *createMessageQueueJob(const KCalCore::IncidenceBase::Ptr &incidence, const KPIMIdentities::Identity &identity, QObject *parent = 0)
    {
        Q_UNUSED(incidence);
        Q_UNUSED(identity);
        return new FakeMessageQueueJob(parent);
    }
};



void ITIPHandlerTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();
    m_pendingItipMessageSignal = 0;
    m_pendingIncidenceChangerSignal = 0;
    m_itipHandler = 0;
    m_cancelExpected = false;
    m_changer = new IncidenceChanger(new FakeITIPHandlerComponentFactory(this), this);
    m_changer->setHistoryEnabled(false);
    m_changer->setGroupwareCommunication(true);
    m_changer->setInvitationPolicy(IncidenceChanger::InvitationPolicySend); // don't show dialogs

    connect(m_changer, SIGNAL(createFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
            SLOT(onCreateFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)));

    connect(m_changer, SIGNAL(deleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)),
            SLOT(onDeleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)));

    connect(m_changer,SIGNAL(modifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
            SLOT(onModifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)));
}

void ITIPHandlerTest::testProcessITIPMessage_data()
{
    QTest::addColumn<QString>("data_filename");
    QTest::addColumn<QString>("action");
    QTest::addColumn<QString>("receiver");
    QTest::addColumn<QString>("incidenceUid"); // uid of incidence in invitation
    QTest::addColumn<Akonadi::ITIPHandler::Result>("expectedResult");
    QTest::addColumn<int>("expectedNumIncidences");
    QTest::addColumn<KCalCore::Attendee::PartStat>("expectedPartStat");

    QString data_filename;
    QString action = QLatin1String("accepted");
    QString incidenceUid = QString::fromLatin1("uosj936i6arrtl9c2i5r2mfuvg");
    QString receiver = QLatin1String(s_ourEmail);
    Akonadi::ITIPHandler::Result expectedResult;
    int expectedNumIncidences = 0;
    KCalCore::Attendee::PartStat expectedPartStat;

    //----------------------------------------------------------------------------------------------
    // Someone invited us to an event, and we accept
    expectedResult = ITIPHandler::ResultSuccess;
    data_filename = QLatin1String("invited_us");
    expectedNumIncidences = 1;
    expectedPartStat = KCalCore::Attendee::Accepted;
    action = QLatin1String("accepted");
    QTest::newRow("invited us1") << data_filename << action << receiver << incidenceUid
                                 << expectedResult
                                 << expectedNumIncidences
                                 << expectedPartStat;
    //----------------------------------------------------------------------------------------------
    // Someone invited us to an event, and we accept conditionally
    expectedResult = ITIPHandler::ResultSuccess;
    data_filename = QLatin1String("invited_us");
    expectedNumIncidences = 1;
    expectedPartStat = KCalCore::Attendee::Tentative;
    action = QLatin1String("tentative");
    QTest::newRow("invited us2") << data_filename << action << receiver << incidenceUid
                                 << expectedResult
                                 << expectedNumIncidences
                                 << expectedPartStat;
    //----------------------------------------------------------------------------------------------
    // Someone invited us to an event, we delegate it
    expectedResult = ITIPHandler::ResultSuccess;
    data_filename = QLatin1String("invited_us");

    // The e-mail to the delegate is sent by kmail's text_calendar.cpp
    expectedNumIncidences = 1;
    expectedPartStat = KCalCore::Attendee::Delegated;
    action = QLatin1String("delegated");
    QTest::newRow("invited us3") << data_filename << action << receiver << incidenceUid
                                 << expectedResult
                                 << expectedNumIncidences
                                 << expectedPartStat;
    //----------------------------------------------------------------------------------------------
    // Process a CANCEL without having the incidence in our calendar.
    // itiphandler should return success and not error
    expectedResult = ITIPHandler::ResultSuccess;
    data_filename = QLatin1String("invited_us");
    expectedNumIncidences = 0;
    action = QLatin1String("cancel");
    QTest::newRow("invited us4") << data_filename << action << receiver << incidenceUid
                                 << expectedResult
                                 << expectedNumIncidences
                                 << expectedPartStat;
    //----------------------------------------------------------------------------------------------
    // Process a REQUEST without having the incidence in our calendar.
    // itiphandler should return success and add the rquest to a calendar
    expectedResult = ITIPHandler::ResultSuccess;
    data_filename = QLatin1String("invited_us");
    expectedNumIncidences = 1;
    expectedPartStat = KCalCore::Attendee::NeedsAction;
    action = QLatin1String("request");
    QTest::newRow("invited us5") << data_filename << action << receiver << incidenceUid
                                 << expectedResult
                                 << expectedNumIncidences
                                 << expectedPartStat;
    //----------------------------------------------------------------------------------------------
    // Here we're testing an error case, where data is null.
    expectedResult = ITIPHandler::ResultError;
    expectedNumIncidences = 0;
    action = QLatin1String("accepted");
    QTest::newRow("invalid data") << QString() << action << receiver << incidenceUid
                                  << expectedResult
                                  << expectedNumIncidences
                                  << expectedPartStat;
    //----------------------------------------------------------------------------------------------
    // Testing invalid action
    expectedResult = ITIPHandler::ResultError;
    data_filename = QLatin1String("invitation_us");
    expectedNumIncidences = 0;
    action = QLatin1String("accepted");
    QTest::newRow("invalid action") << data_filename << QString() << receiver << incidenceUid
                                    << expectedResult
                                    << expectedNumIncidences
                                    << expectedPartStat;
    //----------------------------------------------------------------------------------------------
    // Test bug 235749
    expectedResult = ITIPHandler::ResultSuccess;
    data_filename = QLatin1String("bug235749");
    expectedNumIncidences = 1;
    expectedPartStat = KCalCore::Attendee::Accepted;
    action = QLatin1String("accepted");
    incidenceUid = QLatin1String("b6f0466a-8877-49d0-a4fc-8ee18ffd8e07"); // Don't change, hardcoded in data file
    QTest::newRow("bug 235749") << data_filename << action << receiver << incidenceUid
                                << expectedResult
                                << expectedNumIncidences
                                << expectedPartStat;
    //----------------------------------------------------------------------------------------------
    // Test counterproposal without a UI delegat set
    expectedResult = ITIPHandler::ResultError;
    data_filename = QLatin1String("invited_us");
    expectedNumIncidences = 0;
    expectedPartStat = KCalCore::Attendee::Accepted;
    action = QLatin1String("counter");
    incidenceUid = QLatin1String("b6f0466a-8877-49d0-a4fc-8ee18ffd8e07");
    QTest::newRow("counter error") << data_filename << action << receiver << incidenceUid
                                   << expectedResult
                                   << expectedNumIncidences
                                   << expectedPartStat;
    //----------------------------------------------------------------------------------------------
}

void ITIPHandlerTest::testProcessITIPMessage()
{
    QFETCH(QString, data_filename);
    QFETCH(QString, action);
    QFETCH(QString, receiver);
    QFETCH(QString, incidenceUid);
    QFETCH(Akonadi::ITIPHandler::Result, expectedResult);
    QFETCH(int, expectedNumIncidences);
    QFETCH(KCalCore::Attendee::PartStat, expectedPartStat);

    FakeMessageQueueJob::sUnitTestResults.clear();
    createITIPHandler();

    m_expectedResult = expectedResult;

    QString iCalData = icalData(data_filename);
    Akonadi::Item::List items;
    processItip(iCalData, receiver, action, expectedNumIncidences, items);

    if (expectedNumIncidences == 1) {
        KCalCore::Incidence::Ptr incidence = items.first().payload<KCalCore::Incidence::Ptr>();
        QVERIFY(incidence);
        QCOMPARE(incidence->schedulingID(), incidenceUid);
        QVERIFY(incidence->schedulingID() != incidence->uid());

        KCalCore::Attendee::Ptr me = ourAttendee(incidence);
        QVERIFY(me);
        QCOMPARE(me->status(), expectedPartStat);
    }

    cleanup();
}

void ITIPHandlerTest::testProcessITIPMessages_data()
{
    QTest::addColumn<QStringList>("invitation_filenames"); // filename to create incidence (inputs)
    QTest::addColumn<QString>("expected_filename"); // filename with expected data   (reference)
    QTest::addColumn<QStringList>("actions"); // we must specify the METHOD. This is an ITipHandler API workaround, not sure why we must pass it as argument since it's already inside the icaldata.
    QStringList invitation_filenames;
    QString expected_filename;
    QStringList actions;
    actions << QLatin1String("accepted") << QLatin1String("accepted");

    //----------------------------------------------------------------------------------------------
    // Someone invited us to an event, we accept, then organizer changes event, and we record update:
    invitation_filenames.clear();
    invitation_filenames << QLatin1String("invited_us") << QLatin1String("invited_us_update01");
    expected_filename = QLatin1String("expected_data/update1");
    QTest::newRow("accept update") << invitation_filenames << expected_filename << actions;
    //----------------------------------------------------------------------------------------------
    // Someone invited us to an event, we accept, then organizer changes event, and we record update:
    invitation_filenames.clear();
    invitation_filenames << QLatin1String("invited_us") << QLatin1String("invited_us_daily_update01");
    expected_filename = QLatin1String("expected_data/update2");
    QTest::newRow("accept recurringupdate") << invitation_filenames << expected_filename << actions;
    //----------------------------------------------------------------------------------------------
    // We accept a recurring event, then the organizer changes the summary to the second instance (RECID)
    expected_filename = QLatin1String("expected_data/update3");
    invitation_filenames.clear();
    invitation_filenames << QLatin1String("invited_us_daily") << QLatin1String("invited_us_daily_update_recid01");
    QTest::newRow("accept recid update") << invitation_filenames << expected_filename << actions;
    //----------------------------------------------------------------------------------------------
    // We accept a recurring event, then we accept a CANCEL with recuring-id.
    // The result is that a exception with status CANCELLED should be created, and our main incidence
    // should not be touched
    invitation_filenames.clear();
    invitation_filenames << QLatin1String("invited_us_daily") << QLatin1String("invited_us_daily_cancel_recid01");
    expected_filename = QLatin1String("expected_data/cancel1");
    actions << QLatin1String("accepted") << QLatin1String("cancel");
    QTest::newRow("accept recid cancel") << invitation_filenames << expected_filename << actions;

    //----------------------------------------------------------------------------------------------
}

void ITIPHandlerTest::testProcessITIPMessages()
{
    QFETCH(QStringList, invitation_filenames);
    QFETCH(QString, expected_filename);
    QFETCH(QStringList, actions);

    const QString receiver = QLatin1String(s_ourEmail);

    FakeMessageQueueJob::sUnitTestResults.clear();
    createITIPHandler();

    m_expectedResult = Akonadi::ITIPHandler::ResultSuccess;

    for (int i=0; i<invitation_filenames.count(); i++) {
        // First accept the invitation that creates the incidence:
        QString iCalData = icalData(invitation_filenames.at(i));
        Item::List items;
        qDebug() << "Processing " << invitation_filenames.at(i);
        processItip(iCalData, receiver, actions.at(i), -1, items);
    }


    QString expectedICalData = icalData(expected_filename);
    KCalCore::MemoryCalendar::Ptr expectedCalendar = KCalCore::MemoryCalendar::Ptr(new KCalCore::MemoryCalendar(KDateTime::UTC));
    KCalCore::ICalFormat format;
    format.fromString(expectedCalendar, expectedICalData);
    compareCalendars(expectedCalendar); // Here's where the cool and complex comparations are done

    cleanup();
}

void ITIPHandlerTest::testProcessITIPMessageCancel_data()
{
    QTest::addColumn<QString>("creation_data_filename"); // filename to create incidence
    QTest::addColumn<QString>("cancel_data_filename"); // filename with incidence cancelation
    QTest::addColumn<QString>("incidenceUid"); // uid of incidence in invitation


    QString creation_data_filename;
    QString cancel_data_filename;
    QString incidenceUid = QString::fromLatin1("uosj936i6arrtl9c2i5r2mfuvg");
    //----------------------------------------------------------------------------------------------
    // Someone invited us to an event, we accept, then organizer cancels event
    creation_data_filename = QLatin1String("invited_us");
    cancel_data_filename = QLatin1String("invited_us_cancel01");

    QTest::newRow("cancel1") << creation_data_filename << cancel_data_filename
                             << incidenceUid;
    //----------------------------------------------------------------------------------------------
    // Someone invited us to daily event, we accept, then organizer cancels the whole recurrence series
    creation_data_filename = QLatin1String("invited_us_daily");
    cancel_data_filename = QLatin1String("invited_us_daily_cancel01");

    QTest::newRow("cancel_daily") << creation_data_filename << cancel_data_filename
                                  << incidenceUid;
    //----------------------------------------------------------------------------------------------
}

void ITIPHandlerTest::testProcessITIPMessageCancel()
{
    QFETCH(QString, creation_data_filename);
    QFETCH(QString, cancel_data_filename);
    QFETCH(QString, incidenceUid);

    const QString receiver = QLatin1String(s_ourEmail);
    FakeMessageQueueJob::sUnitTestResults.clear();
    createITIPHandler();

    m_expectedResult = Akonadi::ITIPHandler::ResultSuccess;

    // First accept the invitation that creates the incidence:
    QString iCalData = icalData(creation_data_filename);
    Item::List items;
    processItip(iCalData, receiver, QLatin1String("accepted"), 1, items);

    KCalCore::Incidence::Ptr incidence = items.first().payload<KCalCore::Incidence::Ptr>();
    QVERIFY(incidence);

    // good, now accept the invitation that has the CANCEL
    iCalData = icalData(cancel_data_filename);
    processItip(iCalData, receiver, QLatin1String("accepted"), 0, items);
}

void ITIPHandlerTest::testOutgoingInvitations_data()
{
    QTest::addColumn<Akonadi::Item>("item"); // existing incidence that will be target of creation, deletion or modification
    QTest::addColumn<Akonadi::IncidenceChanger::ChangeType>("changeType"); // creation, deletion, modification
    QTest::addColumn<int>("expectedEmailCount");
    QTest::addColumn<IncidenceChanger::InvitationPolicy>("invitationPolicy");
    QTest::addColumn<bool>("userCancels");

    const bool userDoesntCancel = false;
    const bool userCancels      = true;

    Akonadi::Item item;
    KCalCore::Incidence::Ptr incidence;
    IncidenceChanger::ChangeType changeType;
    const IncidenceChanger::InvitationPolicy invitationPolicyAsk     = IncidenceChanger::InvitationPolicyAsk;
    const IncidenceChanger::InvitationPolicy invitationPolicySend     = IncidenceChanger::InvitationPolicySend;
    const IncidenceChanger::InvitationPolicy invitationPolicyDontSend = IncidenceChanger::InvitationPolicyDontSend;
    int expectedEmailCount = 0;
    Q_UNUSED(invitationPolicyAsk);

    const QString ourEmail     = QLatin1String(s_ourEmail);
    const Attendee::Ptr us = Attendee::Ptr(new Attendee(QString(), ourEmail));
    const Attendee::Ptr mia = Attendee::Ptr(new Attendee(QLatin1String("Mia Wallace"), QLatin1String("mia@dev.nul")));
    const Attendee::Ptr vincent = Attendee::Ptr(new Attendee(QLatin1String("Vincent"), QLatin1String("vincent@dev.nul")));
    const Attendee::Ptr jules = Attendee::Ptr(new Attendee(QLatin1String("Jules"), QLatin1String("jules@dev.nul")));
    const QString uid = QLatin1String("random-uid-123");

    //----------------------------------------------------------------------------------------------
    // Creation. We are organizer. We invite another person.
    changeType = IncidenceChanger::ChangeTypeCreate;
    item = generateIncidence(uid, /**organizer=*/ourEmail);
    incidence = item.payload<KCalCore::Incidence::Ptr>();
    incidence->addAttendee(vincent);
    incidence->addAttendee(jules);
    expectedEmailCount = 1;
    QTest::newRow("Creation. We organize.") << item << changeType << expectedEmailCount
                                            << invitationPolicySend << userDoesntCancel;
    //----------------------------------------------------------------------------------------------
    // Creation. We are organizer. We invite another person. But we choose not to send invitation e-mail.
    changeType = IncidenceChanger::ChangeTypeCreate;
    item = generateIncidence(uid, /**organizer=*/ourEmail);
    incidence = item.payload<KCalCore::Incidence::Ptr>();
    incidence->addAttendee(vincent);
    incidence->addAttendee(jules);
    expectedEmailCount = 0;
    QTest::newRow("Creation. We organize.2") << item << changeType << expectedEmailCount << invitationPolicyDontSend << userDoesntCancel;
    //----------------------------------------------------------------------------------------------
    // We delete an event that we organized, and has attendees, that will be notified.
    changeType = IncidenceChanger::ChangeTypeDelete;
    item = generateIncidence(uid, /**organizer=*/ourEmail);
    incidence = item.payload<KCalCore::Incidence::Ptr>();
    incidence->addAttendee(vincent);
    incidence->addAttendee(jules);
    expectedEmailCount = 1;
    QTest::newRow("Deletion. We organized.") << item << changeType << expectedEmailCount << invitationPolicySend << userDoesntCancel;
    //----------------------------------------------------------------------------------------------
    // We delete an event that we organized, and has attendees. We won't send e-mail notifications.
    changeType = IncidenceChanger::ChangeTypeDelete;
    item = generateIncidence(uid, /**organizer=*/ourEmail);
    incidence = item.payload<KCalCore::Incidence::Ptr>();
    incidence->addAttendee(vincent);
    incidence->addAttendee(jules);
    expectedEmailCount = 0;
    QTest::newRow("Deletion. We organized.2") << item << changeType << expectedEmailCount << invitationPolicyDontSend << userDoesntCancel;
    //----------------------------------------------------------------------------------------------
    // We delete an event that we organized, and has attendees, who will be notified.
    changeType = IncidenceChanger::ChangeTypeModify;
    item = generateIncidence(uid, /**organizer=*/ourEmail);
    incidence = item.payload<KCalCore::Incidence::Ptr>();
    incidence->addAttendee(vincent);
    incidence->addAttendee(jules);
    expectedEmailCount = 1;
    QTest::newRow("Modification. We organizd.") << item << changeType << expectedEmailCount << invitationPolicySend << userDoesntCancel;
    //----------------------------------------------------------------------------------------------
    // We delete an event that we organized, and has attendees, who wont be notified.
    changeType = IncidenceChanger::ChangeTypeModify;
    item = generateIncidence(uid, /**organizer=*/ourEmail);
    incidence = item.payload<KCalCore::Incidence::Ptr>();
    incidence->addAttendee(vincent); // TODO: test that all attendees got the e-mail
    incidence->addAttendee(jules);
    expectedEmailCount = 0;
    QTest::newRow("Modification. We organizd.2") << item << changeType << expectedEmailCount << invitationPolicyDontSend << userDoesntCancel;
    //----------------------------------------------------------------------------------------------
    // We delete an event which we're not the organizer of. Organizer gets REPLY with PartState=Declined
    changeType = IncidenceChanger::ChangeTypeDelete;
    item = generateIncidence(uid, /**organizer=*/mia->email());
    incidence = item.payload<KCalCore::Incidence::Ptr>();
    incidence->addAttendee(vincent);
    incidence->addAttendee(jules);
    us->setStatus(Attendee::Accepted); // TODO: Test without accepted status
    incidence->addAttendee(us); // TODO: test that attendees didn't receive the REPLY
    expectedEmailCount = 1; // REPLY is always sent, there are no dialogs to control this. Dialogs only control REQUEST and CANCEL. Bug or feature ?
    QTest::newRow("Deletion. We didnt organize.") << item << changeType << expectedEmailCount << invitationPolicyDontSend << userDoesntCancel;
    //----------------------------------------------------------------------------------------------
    // We delete an event which we're not the organizer of. Organizer gets REPLY with PartState=Declined
    changeType = IncidenceChanger::ChangeTypeDelete;
    item = generateIncidence(uid, /**organizer=*/mia->email());
    incidence = item.payload<KCalCore::Incidence::Ptr>();
    incidence->addAttendee(vincent);
    incidence->addAttendee(jules); // TODO: test that attendees didn't receive the REPLY
    us->setStatus(Attendee::Accepted); // TODO: Test without accepted status
    incidence->addAttendee(us);
    expectedEmailCount = 1;
    QTest::newRow("Deletion. We didnt organize.2") << item << changeType << expectedEmailCount << invitationPolicySend << userDoesntCancel;
    //----------------------------------------------------------------------------------------------
    // We modified an event which we're not the organizer of. And, when the "do you really want to modify", we choose "yes".
    changeType = IncidenceChanger::ChangeTypeModify;
    item = generateIncidence(uid, /**organizer=*/mia->email());
    incidence = item.payload<KCalCore::Incidence::Ptr>();
    incidence->addAttendee(vincent);
    incidence->addAttendee(jules);
    us->setStatus(Attendee::Accepted);
    incidence->addAttendee(us);
    expectedEmailCount = 0;
    QTest::newRow("Modification. We didnt organize") << item << changeType << expectedEmailCount << invitationPolicySend << userDoesntCancel;
    //----------------------------------------------------------------------------------------------
    // We modified an event which we're not the organizer of. And, when the "do you really want to modify", we choose "no".
    changeType = IncidenceChanger::ChangeTypeModify;
    item = generateIncidence(uid, /**organizer=*/mia->email());
    incidence = item.payload<KCalCore::Incidence::Ptr>();
    incidence->addAttendee(vincent);
    incidence->addAttendee(jules);
    us->setStatus(Attendee::Accepted);
    incidence->addAttendee(us);
    expectedEmailCount = 0;
    QTest::newRow("Modification. We didnt organize.2") << item << changeType << expectedEmailCount << invitationPolicyDontSend << userCancels;
    //----------------------------------------------------------------------------------------------
}

void ITIPHandlerTest::testOutgoingInvitations()
{
    QFETCH(Akonadi::Item, item);
    QFETCH(IncidenceChanger::ChangeType, changeType);
    QFETCH(int, expectedEmailCount);
    QFETCH(IncidenceChanger::InvitationPolicy, invitationPolicy);
    QFETCH(bool, userCancels);
    KCalCore::Incidence::Ptr incidence = item.payload<KCalCore::Incidence::Ptr>();

    m_pendingIncidenceChangerSignal = 1;
    FakeMessageQueueJob::sUnitTestResults.clear();
    m_changer->setInvitationPolicy(invitationPolicy);

    m_cancelExpected = userCancels;

    switch (changeType) {
    case IncidenceChanger::ChangeTypeCreate:
        m_changer->createIncidence(incidence, mCollection);
        waitForIt();
        QCOMPARE(FakeMessageQueueJob::sUnitTestResults.count(), expectedEmailCount);
        break;
    case IncidenceChanger::ChangeTypeModify: {
        // Create if first, so we have something to modify
        m_changer->setGroupwareCommunication(false); // we disable groupware because creating an incidence which we're not the organizer of is not permitted.
        m_changer->createIncidence(incidence, mCollection);
        waitForIt();
        m_changer->setGroupwareCommunication(true);
        QCOMPARE(FakeMessageQueueJob::sUnitTestResults.count(), 0);
        QVERIFY(mLastInsertedItem.isValid());

        m_pendingIncidenceChangerSignal = 1;
        Incidence::Ptr oldIncidence = Incidence::Ptr(incidence->clone());
        incidence->setSummary(QLatin1String("the-new-summary"));
        int changeId = m_changer->modifyIncidence(mLastInsertedItem, oldIncidence);
        QVERIFY(changeId != 1);
        waitForIt();
        QCOMPARE(FakeMessageQueueJob::sUnitTestResults.count(), expectedEmailCount);
    }
    break;
    case IncidenceChanger::ChangeTypeDelete:
        // Create if first, so we have something to delete
        m_changer->setGroupwareCommunication(false);
        m_changer->createIncidence(incidence, mCollection);
        waitForIt();
        m_changer->setGroupwareCommunication(true);
        QCOMPARE(FakeMessageQueueJob::sUnitTestResults.count(), 0);

        QVERIFY(mLastInsertedItem.isValid());
        m_pendingIncidenceChangerSignal = 1;
        m_changer->deleteIncidence(mLastInsertedItem);
        waitForIt();
        QCOMPARE(FakeMessageQueueJob::sUnitTestResults.count(), expectedEmailCount);
        break;
    default:
        Q_ASSERT(false);
    }

}

void ITIPHandlerTest::testIdentity_data()
{
    QTest::addColumn<QString>("email");
    QTest::addColumn<bool>("expectedResult");

    const QString myEmail = QLatin1String(s_ourEmail);
    QString myEmail2      = QString::fromLatin1("Some name <%1>").arg(myEmail);

    const QString myAlias1    = QLatin1String("alias1@kde.org"); // hardcoded in emailidentities, do not change
    const QString myIdentity2 = QLatin1String(s_outEmail2);


    QTest::newRow("Me")           << myEmail     << true;
    QTest::newRow("Also me")      << myEmail2    << true;
    QTest::newRow("My identity2") << myIdentity2 << true;
    QTest::newRow("Not me")       << QString::fromLatin1("laura.palmer@twinpeaks.com") << false;

    QTest::newRow("My alias") << myAlias1 << true;

}

void ITIPHandlerTest::testIdentity()
{
    QFETCH(QString, email);
    QFETCH(bool, expectedResult);

    if (CalendarUtils::thatIsMe(email) != expectedResult) {
        qDebug() << email;
        QVERIFY(false);
    }
}

void ITIPHandlerTest::cleanup()
{
    Akonadi::Item::List items = calendarItems();
    foreach(const Akonadi::Item &item, items) {
        ItemDeleteJob *job = new ItemDeleteJob(item);
        AKVERIFYEXEC(job);
    }

    delete m_itipHandler;
    m_itipHandler = 0;
}

void ITIPHandlerTest::createITIPHandler()
{
    m_itipHandler = new Akonadi::ITIPHandler(new FakeITIPHandlerComponentFactory(this), this);
    m_itipHandler->setShowDialogsOnError(false);
    connect(m_itipHandler, SIGNAL(iTipMessageProcessed(Akonadi::ITIPHandler::Result,QString)),
            SLOT(oniTipMessageProcessed(Akonadi::ITIPHandler::Result,QString)));
}

QString ITIPHandlerTest::icalData(const QString &data_filename)
{
    QString absolutePath = QLatin1String(ITIP_DATA_DIR) + QLatin1Char('/') + data_filename;
    return QString::fromLatin1(readFile(absolutePath));
}

void ITIPHandlerTest::processItip(const QString &icaldata, const QString &receiver,
                                  const QString &action, int expectedNumIncidences,
                                  Akonadi::Item::List &items)
{
    items.clear();
    m_pendingItipMessageSignal = 1;
    m_itipHandler->processiTIPMessage(receiver, icaldata, action);
    waitForIt();

    // 0 e-mails are sent because the status update e-mail is sent by
    // kmail's text_calendar.cpp.
    QCOMPARE(FakeMessageQueueJob::sUnitTestResults.count(), 0);

    items = calendarItems();

    if (expectedNumIncidences != -1) {
        QCOMPARE(items.count(), expectedNumIncidences);
    }
}

Attendee::Ptr ITIPHandlerTest::ourAttendee(const KCalCore::Incidence::Ptr &incidence) const
{
    KCalCore::Attendee::List attendees = incidence->attendees();
    KCalCore::Attendee::Ptr me;
    foreach(const KCalCore::Attendee::Ptr &attendee, attendees) {
        if (attendee->email() == QLatin1String(s_ourEmail)) {
            me = attendee;
            break;
        }
    }

    return me;
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

void ITIPHandlerTest::onCreateFinished(int changeId, const Item &item,
                                       IncidenceChanger::ResultCode resultCode,
                                       const QString &errorString)
{
    Q_UNUSED(changeId);
    Q_UNUSED(errorString);
    mLastInsertedItem = item;
    m_pendingIncidenceChangerSignal--;
    QVERIFY(m_pendingIncidenceChangerSignal >= 0);
    if (m_pendingIncidenceChangerSignal == 0) {
        stopWaiting();
    }
    QCOMPARE(resultCode, IncidenceChanger::ResultCodeSuccess);
}

void ITIPHandlerTest::onDeleteFinished(int changeId, const QVector<Entity::Id> &deletedIds,
                                       IncidenceChanger::ResultCode resultCode,
                                       const QString &errorString)
{
    Q_UNUSED(changeId);
    Q_UNUSED(errorString);
    Q_UNUSED(deletedIds);

    m_pendingIncidenceChangerSignal--;
    QVERIFY(m_pendingIncidenceChangerSignal >= 0);
    if (m_pendingIncidenceChangerSignal == 0) {
        stopWaiting();
    }
    QCOMPARE(resultCode, IncidenceChanger::ResultCodeSuccess);
}

void ITIPHandlerTest::onModifyFinished(int changeId, const Item &item,
                                       IncidenceChanger::ResultCode resultCode,
                                       const QString &errorString)
{
    Q_UNUSED(changeId);
    Q_UNUSED(errorString);
    Q_UNUSED(item);

    m_pendingIncidenceChangerSignal--;
    QVERIFY(m_pendingIncidenceChangerSignal >= 0);
    if (m_pendingIncidenceChangerSignal == 0) {
        stopWaiting();
    }
    QCOMPARE(resultCode, m_cancelExpected ? IncidenceChanger::ResultCodeUserCanceled
             : IncidenceChanger::ResultCodeSuccess);
}


QTEST_AKONADIMAIN(ITIPHandlerTest, GUI)
