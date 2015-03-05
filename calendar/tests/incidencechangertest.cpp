/*
    Copyright (c) 2010-2011 Sérgio Martins <iamsergio@gmail.com>

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

#include "../incidencechanger.h"

#include <akonadi/collection.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/qtest_akonadi.h>

#include <akonadi/item.h>
#include <akonadi/collection.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemdeletejob.h>

#include <kcalcore/event.h>
#include <kcalcore/journal.h>
#include <kcalcore/todo.h>

#include <QBitArray>

using namespace Akonadi;
using namespace KCalCore;

Q_DECLARE_METATYPE(QList<Akonadi::IncidenceChanger::ChangeType>)
Q_DECLARE_METATYPE(QList<bool>)
Q_DECLARE_METATYPE(QList<Akonadi::Collection::Right>)
Q_DECLARE_METATYPE(QList<Akonadi::Collection::Rights>)
Q_DECLARE_METATYPE(QList<Akonadi::IncidenceChanger::ResultCode>)
Q_DECLARE_METATYPE(KCalCore::RecurrenceRule::PeriodType)

QString s_ourEmail = QLatin1String("unittests@dev.nul"); // change also in kdepimlibs/akonadi/calendar/tests/unittestenv/kdehome/share/config
QString s_outEmail2 = QLatin1String("identity2@kde.org");

static Akonadi::Item item()
{
    Item item;
    Incidence::Ptr incidence = Incidence::Ptr(new Event());
    incidence->setSummary(QLatin1String("random summary"));
    item.setMimeType(incidence->mimeType());
    item.setPayload<KCalCore::Incidence::Ptr>(incidence);
    return item;
}

static Akonadi::Item createItem(const Akonadi::Collection &collection)
{
    Item i = item();
    ItemCreateJob *createJob = new ItemCreateJob(i, collection);

    createJob->exec();
    return createJob->item();
}

class IncidenceChangerTest : public QObject
{
    Q_OBJECT
    Collection mCollection;

    QHash<int,IncidenceChanger::ResultCode> mExpectedResultByChangeId;
    IncidenceChanger *mChanger;

    int mIncidencesToDelete;
    int mIncidencesToAdd;
    int mIncidencesToModify;

    QHash<int,Akonadi::Item::Id> mItemIdByChangeId;
    QHash<QString,Akonadi::Item::Id> mItemIdByUid;
    int mChangeToWaitFor;
    bool mPermissionsOrRollback;
    bool mDiscardedEqualsSuccess;
    Akonadi::Item mLastItemCreated;

private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();

        mIncidencesToDelete = 0;
        mIncidencesToAdd    = 0;
        mIncidencesToModify = 0;
        mPermissionsOrRollback = false;
        mDiscardedEqualsSuccess = false;

        mChangeToWaitFor = -1;
        //Control::start(); //TODO: uncomment when using testrunner
        qRegisterMetaType<Akonadi::Item>("Akonadi::Item");
        qRegisterMetaType<QList<Akonadi::IncidenceChanger::ChangeType> >("QList<Akonadi::IncidenceChanger::ChangeType>");
        CollectionFetchJob *job = new CollectionFetchJob(Collection::root(),
                CollectionFetchJob::Recursive,
                this);
        // Get list of collections
        job->fetchScope().setContentMimeTypes(QStringList() << QLatin1String("application/x-vnd.akonadi.calendar.event"));
        AKVERIFYEXEC(job);

        // Find our collection
        Collection::List collections = job->collections();
        QVERIFY(!collections.isEmpty());
        mCollection = collections.first();
        mCollection.setRights(Collection::Rights(Collection::AllRights));

        QVERIFY(mCollection.isValid());
        QVERIFY((mCollection.rights() & Akonadi::Collection::CanCreateItem));

        mChanger = new IncidenceChanger(this);
        mChanger->setShowDialogsOnError(false);

        connect(mChanger, SIGNAL(createFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
                SLOT(createFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)));

        connect(mChanger, SIGNAL(deleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)),
                SLOT(deleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)));

        connect(mChanger,SIGNAL(modifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
                SLOT(modifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)));
    }

    void testCreating_data()
    {
        QTest::addColumn<bool>("sendInvalidIncidence");
        QTest::addColumn<QString>("uid");
        QTest::addColumn<QString>("summary");
        QTest::addColumn<Akonadi::Collection>("destinationCollection");
        QTest::addColumn<Akonadi::Collection>("defaultCollection");
        QTest::addColumn<bool>("respectsCollectionRights");
        QTest::addColumn<Akonadi::IncidenceChanger::DestinationPolicy>("destinationPolicy");
        QTest::addColumn<bool>("failureExpected");
        QTest::addColumn<Akonadi::IncidenceChanger::ResultCode>("expectedResultCode");

        QTest::newRow("Simple Creation1") << false << "SomeUid1" << "Summary1" << mCollection
                                          << Collection() << true
                                          << IncidenceChanger::DestinationPolicyNeverAsk
                                          << false << IncidenceChanger::ResultCodeSuccess;

        QTest::newRow("Simple Creation2") << false << "SomeUid2" << "Summary2" << mCollection
                                          << Collection() << true
                                          << IncidenceChanger::DestinationPolicyNeverAsk
                                          << false << IncidenceChanger::ResultCodeSuccess;

        QTest::newRow("Invalid incidence") << true << "SomeUid3" << "Summary3" << mCollection
                                           << Collection() << true
                                           << IncidenceChanger::DestinationPolicyNeverAsk
                                           << true;

        QTest::newRow("Invalid collection") << false << "SomeUid4" << "Summary4" << Collection()
                                            << Collection() << true
                                            << IncidenceChanger::DestinationPolicyNeverAsk
                                            << false << IncidenceChanger::ResultCodeInvalidDefaultCollection;

        QTest::newRow("Default collection") << false << "SomeUid5" << "Summary5" << Collection()
                                            << mCollection << true
                                            << IncidenceChanger::DestinationPolicyDefault
                                            << false << IncidenceChanger::ResultCodeSuccess;

        // In this case, the collection dialog shouldn't be shown, as we only have 1 collection
        QTest::newRow("Only one collection") << false << "SomeUid6" << "Summary6" << Collection()
                                             << Collection() << true
                                             << IncidenceChanger::DestinationPolicyAsk
                                             << false << IncidenceChanger::ResultCodeSuccess;

        Collection collectionWithoutRights = Collection(mCollection.id());
        collectionWithoutRights.setRights(Collection::Rights());
        Q_ASSERT((mCollection.rights() & Akonadi::Collection::CanCreateItem));

        QTest::newRow("No rights") << false << "SomeUid6" << "Summary6" << Collection()
                                   << collectionWithoutRights << true
                                   << IncidenceChanger::DestinationPolicyNeverAsk
                                   << false << IncidenceChanger::ResultCodePermissions;

        QTest::newRow("No rights but its ok") << false << "SomeUid7" << "Summary7" << Collection()
                                              << collectionWithoutRights << false
                                              << IncidenceChanger::DestinationPolicyNeverAsk
                                              << false
                                              << IncidenceChanger::ResultCodeSuccess;
    }

    void testCreating()
    {
        QFETCH(bool, sendInvalidIncidence);
        QFETCH(QString, uid);
        QFETCH(QString, summary);
        QFETCH(Akonadi::Collection, destinationCollection);
        QFETCH(Akonadi::Collection, defaultCollection);
        QFETCH(bool, respectsCollectionRights);
        QFETCH(Akonadi::IncidenceChanger::DestinationPolicy, destinationPolicy);
        QFETCH(bool, failureExpected);

        Incidence::Ptr incidence;

        if (!sendInvalidIncidence) {
            incidence = Incidence::Ptr(new Event());
            incidence->setUid(uid);
            incidence->setSummary(summary);
        }
        mChanger->setRespectsCollectionRights(respectsCollectionRights);
        mChanger->setDestinationPolicy(destinationPolicy);
        mChanger->setDefaultCollection(defaultCollection);
        const int changeId = mChanger->createIncidence(incidence, destinationCollection);

        QVERIFY(failureExpected ^ (changeId != -1));

        if (!failureExpected) {
            QFETCH(Akonadi::IncidenceChanger::ResultCode, expectedResultCode);
            mIncidencesToAdd = 1;

            mExpectedResultByChangeId.insert(changeId, expectedResultCode);
            waitForSignals();

            if (expectedResultCode == IncidenceChanger::ResultCodeSuccess && !failureExpected) {
                Item item;
                QVERIFY(mItemIdByChangeId.contains(changeId));
                item.setId(mItemIdByChangeId.value(changeId));
                ItemFetchJob *fetchJob = new ItemFetchJob(item, this);
                fetchJob->fetchScope().fetchFullPayload();
                AKVERIFYEXEC(fetchJob);
                QVERIFY(!fetchJob->items().isEmpty());
                Item retrievedItem = fetchJob->items().first();
                QVERIFY(retrievedItem.isValid());
                QVERIFY(retrievedItem.hasPayload());
                QVERIFY(retrievedItem.hasPayload<KCalCore::Event::Ptr>());
                QVERIFY(retrievedItem.hasPayload<KCalCore::Incidence::Ptr>());
                Incidence::Ptr incidence = retrievedItem.payload<KCalCore::Incidence::Ptr>();
                QCOMPARE(incidence->summary(), summary);
                QCOMPARE(incidence->uid(), uid);
            }
        }
        mItemIdByChangeId.clear();
    }

    void testDeleting_data()
    {
        QTest::addColumn<Akonadi::Item::List>("items");
        QTest::addColumn<bool>("respectsCollectionRights");
        QTest::addColumn<bool>("failureExpected");
        QTest::addColumn<Akonadi::IncidenceChanger::ResultCode>("expectedResultCode");

        QTest::newRow("Delete empty list")   << Item::List() << true << true;
        QTest::newRow("Delete invalid item") << (Item::List() << Item()) << true << true;

        ItemFetchJob *fetchJob = new ItemFetchJob(mCollection);
        fetchJob->fetchScope().fetchFullPayload();
        AKVERIFYEXEC(fetchJob);
        Item::List items = fetchJob->items();

        // 5 Incidences were created in testCreating(). Keep this in sync.
        QCOMPARE(items.count(), 5);
        QTest::newRow("Simple delete") << (Item::List() << items.at(0)) << true << false
                                       << IncidenceChanger::ResultCodeSuccess;

        QTest::newRow("Delete already deleted") << (Item::List() << items.at(0)) << true
                                                << false
                                                << IncidenceChanger::ResultCodeAlreadyDeleted;

        QTest::newRow("Delete all others") << (Item::List() << items.at(1) << items.at(2))
                                           << true << false << IncidenceChanger::ResultCodeSuccess;

        Collection collectionWithoutRights = Collection(mCollection.id());
        collectionWithoutRights.setRights(Collection::Rights());
        Item item = items.at(3);
        item.setParentCollection(collectionWithoutRights);
        QTest::newRow("Delete can't delete") << (Item::List() << item)
                                             << true << false
                                             << IncidenceChanger::ResultCodePermissions;
    }

    void testDeleting()
    {
        QFETCH(Akonadi::Item::List, items);
        QFETCH(bool, respectsCollectionRights);
        QFETCH(bool, failureExpected);
        mChanger->setRespectsCollectionRights(respectsCollectionRights);
        const int changeId = mChanger->deleteIncidences(items);

        QVERIFY(failureExpected ^ (changeId != -1));

        if (!failureExpected) {
            QFETCH(Akonadi::IncidenceChanger::ResultCode, expectedResultCode);
            mIncidencesToDelete = 1;
            mExpectedResultByChangeId.insert(changeId, expectedResultCode);
            waitForSignals();

            if (expectedResultCode == IncidenceChanger::ResultCodeSuccess) {
                // Check that the incidence was really deleted
                Item item;
                foreach(const Akonadi::Item &item, items) {
                    ItemFetchJob *fetchJob = new ItemFetchJob(item, this);
                    fetchJob->fetchScope().fetchFullPayload();
                    QVERIFY(!fetchJob->exec());
                    QVERIFY(fetchJob->items().isEmpty());
                    delete fetchJob;
                }
            }
        }
    }

    void testModifying_data()
    {
        QTest::addColumn<Akonadi::Item>("item");
        QTest::addColumn<QString>("newSummary");
        QTest::addColumn<bool>("respectsCollectionRights");
        QTest::addColumn<int>("expectedRevision");
        QTest::addColumn<bool>("failureExpected");
        QTest::addColumn<Akonadi::IncidenceChanger::ResultCode>("expectedResultCode");

        QTest::newRow("Invalid item") << Item() << QString() << true << 0 << true;
        QTest::newRow("Valid item, invalid payload") << Item(1) << QString() << true << 0 << true;

        Item item;
        item.setMimeType(Event::eventMimeType());
        Incidence::Ptr incidence = Incidence::Ptr(new Event());
        incidence->setUid(QLatin1String("test123uid"));
        incidence->setSummary(QLatin1String("summary"));
        item.setPayload<KCalCore::Incidence::Ptr>(incidence);
        ItemCreateJob *job = new ItemCreateJob(item, mCollection, this);
        AKVERIFYEXEC(job);
        item = job->item();
        incidence->setSummary(QLatin1String("New Summary"));
        item.setPayload<KCalCore::Incidence::Ptr>(incidence);

        QTest::newRow("Change summary") << item << "New Summary" << true << 1 << false
                                        << IncidenceChanger::ResultCodeSuccess;

        Collection collectionWithoutRights = Collection(mCollection.id());
        collectionWithoutRights.setRights(Collection::Rights());
        item.setParentCollection(collectionWithoutRights);

        QTest::newRow("Can't change") << item << "New Summary" << true << 1 << false
                                      << IncidenceChanger::ResultCodePermissions;
    }

    void testModifying()
    {
        QFETCH(Akonadi::Item, item);
        QFETCH(QString, newSummary);
        QFETCH(bool, respectsCollectionRights);
        QFETCH(int, expectedRevision);
        QFETCH(bool, failureExpected);

        mChanger->setRespectsCollectionRights(respectsCollectionRights);
        const int changeId = mChanger->modifyIncidence(item);
        QVERIFY(failureExpected ^ (changeId != -1));

        if (!failureExpected) {
            QFETCH(Akonadi::IncidenceChanger::ResultCode, expectedResultCode);

            mIncidencesToModify = 1;
            mExpectedResultByChangeId.insert(changeId, expectedResultCode);
            waitForSignals();
            ItemFetchJob *fetchJob = new ItemFetchJob(item, this);
            fetchJob->fetchScope().fetchFullPayload();
            AKVERIFYEXEC(fetchJob);
            QVERIFY(fetchJob->items().count() == 1);
            Item fetchedItem = fetchJob->items().first();
            QVERIFY(fetchedItem.isValid());
            QVERIFY(fetchedItem.hasPayload<KCalCore::Incidence::Ptr>());
            Incidence::Ptr incidence = fetchedItem.payload<KCalCore::Incidence::Ptr>();
            QCOMPARE(incidence->summary(), newSummary);
            QCOMPARE(incidence->revision(), expectedRevision);
            delete fetchJob;
        }
    }

    void testModifyingAlarmSettings()
    {
        // A user should be able to change alarm settings independently
        // do not trigger a revision increment
        // kolab #1386
        Item item;
        item.setMimeType(Event::eventMimeType());
        Incidence::Ptr incidence = Incidence::Ptr(new Event());
        incidence->setUid(QLatin1String("test123uid"));
        incidence->setSummary(QLatin1String("summary"));
        incidence->setOrganizer(Person::Ptr(new Person(QLatin1String("orga"), QLatin1String("orga@dev.nul"))));
        incidence->setDirtyFields(QSet<IncidenceBase::Field>());
        item.setPayload<KCalCore::Incidence::Ptr>(incidence);
        ItemCreateJob *job = new ItemCreateJob(item, mCollection, this);
        AKVERIFYEXEC(job);
        item = job->item();
        Alarm::Ptr alarm = Alarm::Ptr(new Alarm(incidence.data()));
        alarm->setStartOffset(Duration(-15));
        alarm->setType(Alarm::Display);
        incidence->addAlarm(alarm);
        item.setPayload<KCalCore::Incidence::Ptr>(incidence);

        mChanger->setRespectsCollectionRights(true);
        const int changeId = mChanger->modifyIncidence(item);
        QVERIFY(changeId != -1);

        mIncidencesToModify = 1;
        mExpectedResultByChangeId.insert(changeId, IncidenceChanger::ResultCodeSuccess);
        waitForSignals();
        ItemFetchJob *fetchJob = new ItemFetchJob(item, this);
        fetchJob->fetchScope().fetchFullPayload();
        AKVERIFYEXEC(fetchJob);
        QVERIFY(fetchJob->items().count() == 1);
        Item fetchedItem = fetchJob->items().first();
        QVERIFY(fetchedItem.isValid());
        QVERIFY(fetchedItem.hasPayload<KCalCore::Incidence::Ptr>());
        Incidence::Ptr incidence2 = fetchedItem.payload<KCalCore::Incidence::Ptr>();
        QCOMPARE(incidence2->alarms().count(), 1);
        QCOMPARE(incidence2->revision(), 0);
        delete fetchJob;
    }

    void testModifyRescedule_data()
    {
        // When a event is resceduled than all attendees part status should set to NEEDS-ACTION
        // kolab #4533
        QTest::addColumn<Akonadi::Item>("item");
        QTest::addColumn<Event::Ptr>("event");
        QTest::addColumn<bool>("expectReset");

        const Attendee::Ptr us = Attendee::Ptr(new Attendee(QString(), s_ourEmail));
        us->setStatus(Attendee::Accepted);
        const Attendee::Ptr mia = Attendee::Ptr(new Attendee(QLatin1String("Mia Wallace"), QLatin1String("mia@dev.nul")));
        mia->setStatus(Attendee::Declined);
        mia->setRSVP(false);
        const Attendee::Ptr vincent = Attendee::Ptr(new Attendee(QLatin1String("Vincent"), QLatin1String("vincent@dev.nul")));
        vincent->setStatus(Attendee::Delegated);
        const Attendee::Ptr jules = Attendee::Ptr(new Attendee(QLatin1String("Jules"), QLatin1String("jules@dev.nul")));
        jules->setStatus(Attendee::Accepted);
        jules->setRole(Attendee::NonParticipant);

        // we as organizator
        Item item;
        item.setMimeType(Event::eventMimeType());
        Event::Ptr incidence = Event::Ptr(new Event());
        incidence->setUid(QLatin1String("test123uid"));
        incidence->setDtStart(KDateTime(QDate(2006, 1, 8), QTime(12, 0, 0), KDateTime::UTC));
        incidence->setDtEnd(KDateTime(QDate(2006, 1, 8), QTime(14, 0, 0), KDateTime::UTC));
        incidence->setAllDay(false);
        incidence->setLocation(QLatin1String("location"));
        incidence->setOrganizer(Person::Ptr(new Person(QString(), s_ourEmail)));
        incidence->addAttendee(us);
        incidence->addAttendee(mia);
        incidence->addAttendee(vincent);
        incidence->addAttendee(jules);
        incidence->setDirtyFields(QSet<IncidenceBase::Field>());
        item.setPayload<KCalCore::Incidence::Ptr>(incidence);

        {
            Event::Ptr event = Event::Ptr(new Event(*incidence));
            event->setDtStart(KDateTime(QDate(2006, 1, 8), QTime(13, 0, 0), KDateTime::UTC));
            QCOMPARE(event->dirtyFields().count(), 1);
            QTest::newRow("organizator:start Date") << item << event << true;
        }

        {
            Event::Ptr event = Event::Ptr(new Event(*incidence));
            event->setDtEnd(KDateTime(QDate(2006, 1, 8), QTime(13, 0, 0), KDateTime::UTC));
            QCOMPARE(event->dirtyFields().count(), 1);
            QTest::newRow("organizator:end Date") << item << event << true;
        }

        {
            Event::Ptr event = Event::Ptr(new Event(*incidence));
            event->setAllDay(true);
            QCOMPARE(event->dirtyFields().count(), 2);
            QTest::newRow("organizator:allDay") << item << event << true;
        }

        {
            Event::Ptr event = Event::Ptr(new Event(*incidence));
            event->setLocation(QLatin1String("location2"));
            QCOMPARE(event->dirtyFields().count(), 1);
            QTest::newRow("organizator:location") << item << event << true;
        }

        {
            Event::Ptr event = Event::Ptr(new Event(*incidence));
            event->setSummary(QLatin1String("summary"));
            QCOMPARE(event->dirtyFields().count(), 1);
            QTest::newRow("organizator:summary") << item << event << false;
        }

        //we are normal attendee
        Item item2;
        item2.setMimeType(Event::eventMimeType());
        Event::Ptr incidence2 = Event::Ptr(new Event());
        incidence2->setUid(QLatin1String("test123uid"));
        incidence2->setDtStart(KDateTime(QDate(2006, 1, 8), QTime(12, 0, 0), KDateTime::UTC));
        incidence2->setDtEnd(KDateTime(QDate(2006, 1, 8), QTime(14, 0, 0), KDateTime::UTC));
        incidence2->setAllDay(false);
        incidence2->setLocation(QLatin1String("location"));
        incidence2->setOrganizer(Person::Ptr(new Person(QLatin1String("External organizator"), QLatin1String("exorga@dev.nul"))));
        incidence2->addAttendee(us);
        incidence2->addAttendee(mia);
        incidence2->addAttendee(vincent);
        incidence2->addAttendee(jules);
        incidence2->setDirtyFields(QSet<IncidenceBase::Field>());
        item2.setPayload<KCalCore::Incidence::Ptr>(incidence2);

        {
            Event::Ptr event = Event::Ptr(new Event(*incidence2));
            event->setDtStart(KDateTime(QDate(2006, 1, 8), QTime(13, 0, 0), KDateTime::UTC));
            QTest::newRow("attendee:start Date") << item2 << event << false;
        }

        {
            Event::Ptr event = Event::Ptr(new Event(*incidence2));
            event->setDtEnd(KDateTime(QDate(2006, 1, 8), QTime(13, 0, 0), KDateTime::UTC));
            QTest::newRow("attendee:end Date") << item2 << event << false;
        }

        {
            Event::Ptr event = Event::Ptr(new Event(*incidence2));
            event->setAllDay(false);
            QTest::newRow("attendee:allDay") << item2 << event << false;
        }

        {
            Event::Ptr event = Event::Ptr(new Event(*incidence2));
            event->setLocation(QLatin1String("location2"));
            QTest::newRow("attendee:location") << item2 << event << false;
        }

        {
            Event::Ptr event = Event::Ptr(new Event(*incidence2));
            event->setSummary(QLatin1String("summary"));
            QTest::newRow("attendee:summary") << item2 << event << false;
        }

    }

    void testModifyRescedule()
    {
        QFETCH(Akonadi::Item, item);
        QFETCH(Event::Ptr, event);
        QFETCH(bool, expectReset);

        item.setId(-1);
        ItemCreateJob *job = new ItemCreateJob(item, mCollection, this);
        AKVERIFYEXEC(job);
        item = job->item();
        item.setPayload<KCalCore::Incidence::Ptr>(event);

        int revision = event->revision();

        mChanger->setRespectsCollectionRights(true);
        const int changeId = mChanger->modifyIncidence(item);
        QVERIFY(changeId != -1);

        mIncidencesToModify = 1;
        mExpectedResultByChangeId.insert(changeId, IncidenceChanger::ResultCodeSuccess);
        waitForSignals();
        ItemFetchJob *fetchJob = new ItemFetchJob(item, this);
        fetchJob->fetchScope().fetchFullPayload();
        AKVERIFYEXEC(fetchJob);
        QVERIFY(fetchJob->items().count() == 1);
        Item fetchedItem = fetchJob->items().first();

        QVERIFY(fetchedItem.isValid());
        QVERIFY(fetchedItem.hasPayload<KCalCore::Event::Ptr>());
        Event::Ptr incidence = fetchedItem.payload<KCalCore::Event::Ptr>();

        QCOMPARE(incidence->revision(), revision + 1);

        if (expectReset) {
            if (incidence->organizer()->email() == s_ourEmail) {
                QCOMPARE(incidence->attendeeByMail(s_ourEmail)->status(), Attendee::Accepted);
            } else {
                QCOMPARE(incidence->attendeeByMail(s_ourEmail)->status(), Attendee::NeedsAction);
            }
            QCOMPARE(incidence->attendeeByMail(QLatin1String("mia@dev.nul"))->status(), Attendee::NeedsAction);
            QCOMPARE(incidence->attendeeByMail(QLatin1String("mia@dev.nul"))->RSVP(), true);
            QCOMPARE(incidence->attendeeByMail(QLatin1String("vincent@dev.nul"))->status(), Attendee::Delegated);
            QCOMPARE(incidence->attendeeByMail(QLatin1String("vincent@dev.nul"))->RSVP(), false);
            QCOMPARE(incidence->attendeeByMail(QLatin1String("jules@dev.nul"))->status(), Attendee::Accepted);
            QCOMPARE(incidence->attendeeByMail(QLatin1String("jules@dev.nul"))->RSVP(), false);
        } else {
            QCOMPARE(incidence->attendeeByMail(s_ourEmail)->status(), Attendee::Accepted);
            QCOMPARE(incidence->attendeeByMail(QLatin1String("mia@dev.nul"))->status(), Attendee::Declined);
            QCOMPARE(incidence->attendeeByMail(QLatin1String("vincent@dev.nul"))->status(), Attendee::Delegated);
            QCOMPARE(incidence->attendeeByMail(QLatin1String("jules@dev.nul"))->status(), Attendee::Accepted);
        }
        delete fetchJob;
    }
    void testMassModifyForConflicts_data()
    {
        QTest::addColumn<Akonadi::Item>("item");
        QTest::addColumn<bool>("waitForPreviousJob");
        QTest::addColumn<int>("numberOfModifications");

        // Create an incidence
        Item item;
        item.setMimeType(Event::eventMimeType());
        Incidence::Ptr incidence = Incidence::Ptr(new Event());
        incidence->setUid(QLatin1String("test123uid"));
        incidence->setSummary(QLatin1String("summary"));
        item.setPayload<KCalCore::Incidence::Ptr>(incidence);
        ItemCreateJob *job = new ItemCreateJob(item, mCollection, this);
        AKVERIFYEXEC(job);
        item = job->item();

        QTest::newRow("15 modifications in sequence") << item << true  << 15;
        QTest::newRow("15 modifications in parallel") << item << false << 15;
    }

    void testMassModifyForConflicts()
    {
        QFETCH(Akonadi::Item, item);
        QFETCH(bool, waitForPreviousJob);
        QFETCH(int, numberOfModifications);
        mDiscardedEqualsSuccess = true;

        Q_ASSERT(numberOfModifications > 0);

        if (!waitForPreviousJob) {
            mIncidencesToModify = numberOfModifications;
        }

        int changeId = -1;
        for (int i=0; i<numberOfModifications; ++i) {
            Incidence::Ptr incidence = item.payload<KCalCore::Incidence::Ptr>();
            Q_ASSERT(incidence);
            incidence->setSummary(QString::number(i));
            changeId = mChanger->modifyIncidence(item);
            QVERIFY(changeId != -1);

            if (waitForPreviousJob) {
                mIncidencesToModify = 1;
                mExpectedResultByChangeId.insert(changeId, IncidenceChanger::ResultCodeSuccess);
                waitForSignals();
                ItemFetchJob *fetchJob = new ItemFetchJob(item, this);
                fetchJob->fetchScope().fetchFullPayload();
                AKVERIFYEXEC(fetchJob);
                QVERIFY(fetchJob->items().count() == 1);
                QCOMPARE(fetchJob->items().first().payload<KCalCore::Incidence::Ptr>()->summary(),
                         QString::number(i));
            }
        }

        if (!waitForPreviousJob) {
            mIncidencesToModify = numberOfModifications;
            // Wait for the last one
            mExpectedResultByChangeId.insert(changeId, IncidenceChanger::ResultCodeSuccess);
            waitForChange(changeId);
            ItemFetchJob *fetchJob = new ItemFetchJob(item, this);
            fetchJob->fetchScope().fetchFullPayload();
            AKVERIFYEXEC(fetchJob);
            QVERIFY(fetchJob->items().count() == 1);
            QCOMPARE(fetchJob->items().first().payload<KCalCore::Incidence::Ptr>()->summary(),
                     QString::number(numberOfModifications-1));
            if (mIncidencesToModify > 0)
                waitForSignals();
        }

        mDiscardedEqualsSuccess = false;
    }

    void testAtomicOperations_data()
    {
        QTest::addColumn<Akonadi::Item::List>("items");
        QTest::addColumn<QList<Akonadi::IncidenceChanger::ChangeType> >("changeTypes");
        QTest::addColumn<QList<bool> >("failureExpectedList");
        QTest::addColumn<QList<Akonadi::IncidenceChanger::ResultCode> >("expectedResults");
        QTest::addColumn<QList<Akonadi::Collection::Rights> >("rights");
        QTest::addColumn<bool>("permissionsOrRollback");

        Akonadi::Item::List items;
        QList<Akonadi::IncidenceChanger::ChangeType> changeTypes;
        QList<bool> failureExpectedList;
        QList<Akonadi::IncidenceChanger::ResultCode> expectedResults;
        QList<Akonadi::Collection::Rights> rights;
        const Collection::Rights allRights = Collection::AllRights;
        const Collection::Rights noRights = Collection::Rights();
        //------------------------------------------------------------------------------------------
        // Create two incidences, should succeed
        items << item() << item() ;
        changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeCreate;
        failureExpectedList << false << false;
        expectedResults << IncidenceChanger::ResultCodeSuccess << IncidenceChanger::ResultCodeSuccess;
        rights << allRights << allRights;

        QTest::newRow("create two - success ") << items << changeTypes << failureExpectedList
                                               << expectedResults << rights << false;
        //------------------------------------------------------------------------------------------
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeModify << IncidenceChanger::ChangeTypeModify;
        items.clear();
        items << createItem(mCollection) << createItem(mCollection);

        QTest::newRow("modify two - success ") << items << changeTypes << failureExpectedList
                                               << expectedResults << rights << false;
        //------------------------------------------------------------------------------------------
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeDelete << IncidenceChanger::ChangeTypeDelete;
        QTest::newRow("delete two - success ") << items << changeTypes << failureExpectedList
                                               << expectedResults << rights << false;
        //------------------------------------------------------------------------------------------
        // Creation succeeds but deletion doesnt ( invalid item case )
        items.clear();
        items << item() << Item(); // Invalid item on purpose
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeDelete;
        failureExpectedList.clear();
        failureExpectedList << false << true;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodeRolledback;
        rights.clear();
        rights << allRights << allRights;

        QTest::newRow("create,try delete") << items << changeTypes << failureExpectedList
                                           << expectedResults << rights << false;
        //------------------------------------------------------------------------------------------
        // deletion doesn't succeed, but creation does ( invalid item case )
        items.clear();
        items << Item() << item(); // Invalid item on purpose
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeDelete << IncidenceChanger::ChangeTypeCreate;
        failureExpectedList.clear();
        failureExpectedList << true << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodeRolledback;
        rights.clear();
        rights << allRights << allRights;

        QTest::newRow("try delete,create") << items << changeTypes << failureExpectedList
                                           << expectedResults << rights << false;
        //------------------------------------------------------------------------------------------
        // Creation succeeds but deletion doesnt ( valid, inexistant item case )
        items.clear();
        items << item() << Item(10101010);
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeDelete;
        failureExpectedList.clear();
        failureExpectedList << false << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodeJobError;
        rights.clear();
        rights << allRights << allRights;

        QTest::newRow("create,try delete v2") << items << changeTypes << failureExpectedList
                                              << expectedResults << rights << false;
        //------------------------------------------------------------------------------------------
        // deletion doesn't succeed, but creation does ( valid, inexistant item case )
        items.clear();
        items << Item(10101010) << item();
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeDelete << IncidenceChanger::ChangeTypeCreate;
        failureExpectedList.clear();
        failureExpectedList << false << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodeJobError
                        << IncidenceChanger::ResultCodeRolledback;
        rights.clear();
        rights << allRights << allRights;

        QTest::newRow("try delete,create v2") << items << changeTypes << failureExpectedList
                                              << expectedResults << rights << false;
        //------------------------------------------------------------------------------------------
        // deletion doesn't succeed, but creation does ( NO ACL case )
        items.clear();
        items << createItem(mCollection) << item();
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeDelete << IncidenceChanger::ChangeTypeCreate;
        failureExpectedList.clear();
        failureExpectedList << false << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodePermissions
                        << IncidenceChanger::ResultCodeRolledback;
        rights.clear();
        rights << noRights << allRights;

        QTest::newRow("try delete(ACL),create") << items << changeTypes << failureExpectedList
                                                << expectedResults << rights << false;
        //------------------------------------------------------------------------------------------
        //Creation succeeds but deletion doesnt ( NO ACL case )
        items.clear();
        items << item() << createItem(mCollection);
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeDelete;
        failureExpectedList.clear();
        failureExpectedList << false << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodePermissions;
        rights.clear();
        rights << allRights << noRights;

        QTest::newRow("create,try delete(ACL)") << items << changeTypes << failureExpectedList
                                                << expectedResults << rights << false;
        //------------------------------------------------------------------------------------------
        // 1 successfull modification, 1 failed creation
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeModify << IncidenceChanger::ChangeTypeCreate;
        items.clear();
        items << createItem(mCollection) << Item();
        failureExpectedList.clear();
        failureExpectedList << false << true;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodeRolledback;
        rights.clear();
        rights << allRights << allRights;

        QTest::newRow("modify,try create") << items << changeTypes << failureExpectedList
                                           << expectedResults << rights << false;
        //------------------------------------------------------------------------------------------
        // 1 successfull modification, 1 failed creation
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeModify << IncidenceChanger::ChangeTypeCreate;
        items.clear();
        items << createItem(mCollection) << item();
        failureExpectedList.clear();
        failureExpectedList << false << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodePermissions;
        rights.clear();
        rights << allRights << noRights;

        QTest::newRow("modify,try create v2") << items << changeTypes << failureExpectedList
                                              << expectedResults << rights << false;
        //------------------------------------------------------------------------------------------
        // 1 failed creation, 1 successfull modification
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeModify;
        items.clear();
        items << Item() << createItem(mCollection);
        failureExpectedList.clear();
        failureExpectedList << true << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodeRolledback;
        rights.clear();
        rights << allRights << allRights;

        QTest::newRow("try create,modify") << items << changeTypes << failureExpectedList
                                           << expectedResults << rights << false;
        //------------------------------------------------------------------------------------------
        // 1 failed creation, 1 successfull modification
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeModify;
        items.clear();
        items << item() << createItem(mCollection);
        failureExpectedList.clear();
        failureExpectedList << false << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodePermissions
                        << IncidenceChanger::ResultCodeRolledback;
        rights.clear();
        rights << noRights << allRights;

        QTest::newRow("try create,modify v2") << items << changeTypes << failureExpectedList
                                              << expectedResults << rights << false;
        //------------------------------------------------------------------------------------------
        // 4 creations, last one fails
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeCreate
                    << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeCreate;
        items.clear();
        items << item() << item() << item() << item();
        failureExpectedList.clear();
        failureExpectedList << false << false << false << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodePermissions;
        rights.clear();
        rights << allRights << allRights << allRights << noRights;

        QTest::newRow("create 4, last fails") << items << changeTypes << failureExpectedList
                                              << expectedResults << rights << false;
        //------------------------------------------------------------------------------------------
        // 4 creations, first one fails
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeCreate
                    << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeCreate;
        items.clear();
        items << item() << item() << item() << item();
        failureExpectedList.clear();
        failureExpectedList << false << false << false << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodePermissions
                        << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodeRolledback;
        rights.clear();
        rights << noRights << allRights << allRights << allRights;

        QTest::newRow("create 4, first fails") << items << changeTypes << failureExpectedList
                                               << expectedResults << rights << false;
        //------------------------------------------------------------------------------------------
        // 4 creations, second one fails
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeCreate
                    << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeCreate;
        items.clear();
        items << item() << item() << item() << item();
        failureExpectedList.clear();
        failureExpectedList << false << false << false << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodePermissions
                        << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodeRolledback;
        rights.clear();
        rights << allRights << noRights << allRights << allRights;

        QTest::newRow("create 4, second fails") << items << changeTypes << failureExpectedList
                                                << expectedResults << rights << false;
        //------------------------------------------------------------------------------------------
        // 4 fails
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeCreate
                    << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeCreate;
        items.clear();
        items << item() << item() << item() << item();
        failureExpectedList.clear();
        failureExpectedList << false << false << false << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodePermissions
                        << IncidenceChanger::ResultCodePermissions
                        << IncidenceChanger::ResultCodePermissions
                        << IncidenceChanger::ResultCodePermissions;
        rights.clear();
        rights << noRights << noRights << noRights << noRights;

        QTest::newRow("create 4, all fail") << items << changeTypes << failureExpectedList
                                            << expectedResults << rights << true;
        //------------------------------------------------------------------------------------------
        //------------------------------------------------------------------------------------------
        //------------------------------------------------------------------------------------------
        //------------------------------------------------------------------------------------------
    }

    void testAtomicOperations()
    {
        QFETCH(Akonadi::Item::List, items);
        QFETCH(QList<Akonadi::IncidenceChanger::ChangeType>, changeTypes);
        QFETCH(QList<bool>, failureExpectedList);
        QFETCH(QList<Akonadi::IncidenceChanger::ResultCode>, expectedResults);
        QFETCH(QList<Akonadi::Collection::Rights>, rights);
        QFETCH(bool, permissionsOrRollback);

        QCOMPARE(items.count(), changeTypes.count());
        QCOMPARE(items.count(), failureExpectedList.count());
        QCOMPARE(items.count(), expectedResults.count());
        QCOMPARE(items.count(), rights.count());

        mPermissionsOrRollback = permissionsOrRollback;
        mChanger->setDefaultCollection(mCollection);
        mChanger->setRespectsCollectionRights(true);
        mChanger->setDestinationPolicy(IncidenceChanger::DestinationPolicyNeverAsk);
        mChanger->startAtomicOperation();
        mIncidencesToAdd = 0;
        mIncidencesToModify = 0;
        mIncidencesToDelete = 0;
        for (int i=0; i<items.count(); ++i) {
            mCollection.setRights(rights[i]);
            mChanger->setDefaultCollection(mCollection);
            const Akonadi::Item item = items[i];
            int changeId = -1;
            switch (changeTypes[i]) {
            case IncidenceChanger::ChangeTypeCreate:
                changeId = mChanger->createIncidence(item.hasPayload() ? item.payload<KCalCore::Incidence::Ptr>() : Incidence::Ptr());
                if (changeId != -1)
                    ++mIncidencesToAdd;
                break;
            case IncidenceChanger::ChangeTypeDelete:
                changeId = mChanger->deleteIncidence(item);
                if (changeId != -1)
                    ++mIncidencesToDelete;
                break;
            case IncidenceChanger::ChangeTypeModify:
                QVERIFY(item.isValid());
                QVERIFY(item.hasPayload<KCalCore::Incidence::Ptr>());
                item.payload<KCalCore::Incidence::Ptr>()->setSummary(QLatin1String("Changed"));
                changeId = mChanger->modifyIncidence(item);
                if (changeId != -1)
                    ++mIncidencesToModify;
                break;
            default:
                QVERIFY(false);
            }
            QVERIFY(!((changeId == -1) ^ failureExpectedList[i]));
            if (changeId != -1) {
                mExpectedResultByChangeId.insert(changeId, expectedResults[i]);
            }
        }
        mChanger->endAtomicOperation();

        waitForSignals();

        //Validate:
        for (int i=0; i<items.count(); ++i) {
            const bool expectedSuccess = (expectedResults[i] == IncidenceChanger::ResultCodeSuccess);
            mCollection.setRights(rights[i]);

            Akonadi::Item item = items[i];

            switch (changeTypes[i]) {
            case IncidenceChanger::ChangeTypeCreate:
                if (expectedSuccess) {
                    QVERIFY(item.hasPayload<KCalCore::Incidence::Ptr>());
                    Incidence::Ptr incidence = item.payload<KCalCore::Incidence::Ptr>();
                    QVERIFY(incidence);
                    QVERIFY(!incidence->uid().isEmpty());
                    QVERIFY(mItemIdByUid.contains(incidence->uid()));
                    ItemFetchJob *fJob = new ItemFetchJob(Item(mItemIdByUid.value(incidence->uid())));
                    fJob->fetchScope().fetchFullPayload();
                    AKVERIFYEXEC(fJob);
                    QCOMPARE(fJob->items().count(), 1);
                    QVERIFY(fJob->items().first().isValid());
                    QVERIFY(fJob->items().first().hasPayload());
                    QVERIFY(fJob->items().first().hasPayload<KCalCore::Incidence::Ptr>());
                    QCOMPARE(item.payload<KCalCore::Incidence::Ptr>()->uid(),
                             fJob->items().first().payload<KCalCore::Incidence::Ptr>()->uid());
                }
                break;
            case IncidenceChanger::ChangeTypeDelete:
                if (expectedSuccess) {
                    ItemFetchJob *fJob = new ItemFetchJob(Item(item.id()));
                    QVERIFY(!fJob->exec());
                }
                break;
            case IncidenceChanger::ChangeTypeModify:
                if (expectedSuccess) {
                    ItemFetchJob *fJob = new ItemFetchJob(Item(item.id()));
                    fJob->fetchScope().fetchFullPayload();
                    AKVERIFYEXEC(fJob);
                    QCOMPARE(item.payload<KCalCore::Incidence::Ptr>()->summary(),
                             fJob->items().first().payload<KCalCore::Incidence::Ptr>()->summary());
                }
                break;
            default:
                QVERIFY(false);
            }
        }
        mPermissionsOrRollback = false;
    }

    void testAdjustRecurrence_data()
    {
        QTest::addColumn<bool>("allDay");
        QTest::addColumn<KDateTime>("dtStart");
        QTest::addColumn<KDateTime>("dtEnd");
        QTest::addColumn<int>("offsetToMove");
        QTest::addColumn<int>("frequency");
        QTest::addColumn<KCalCore::RecurrenceRule::PeriodType>("recurrenceType");

        // For weekly recurrences
        QTest::addColumn<QBitArray>("weekDays");
        QTest::addColumn<QBitArray>("expectedWeekDays");

        // Recurrence end
        QTest::addColumn<QDate>("recurrenceEnd");
        QTest::addColumn<QDate>("expectedRecurrenceEnd");

        const KDateTime dtStart = KDateTime(QDate::currentDate(), QTime(8, 0));
        const KDateTime dtEnd = dtStart.addSecs(3600);
        const int one_day = 3600*24;
        const int one_hour = 3600;
        QBitArray days(7);
        QBitArray expectedDays(7);

        //-------------------------------------------------------------------------
        days.setBit(dtStart.date().dayOfWeek()-1);
        expectedDays.setBit(dtStart.addSecs(one_day).date().dayOfWeek()-1);

        QTest::newRow("weekly") << false << dtStart << dtEnd << one_day
                                << 1 << KCalCore::RecurrenceRule::rWeekly
                                <<  days << expectedDays << QDate() << QDate();
        //-------------------------------------------------------------------------
        days.fill(false);
        days.setBit(dtStart.date().dayOfWeek()-1);
        expectedDays.setBit(dtStart.addSecs(one_day).date().dayOfWeek()-1);

        QTest::newRow("weekly allday") << true << KDateTime(dtStart.date()) << KDateTime(dtEnd.date())
                                       << one_day << 1 << KCalCore::RecurrenceRule::rWeekly
                                       << days << expectedDays << QDate() << QDate();
        //-------------------------------------------------------------------------
        // Here nothing should change
        days.fill(false);
        days.setBit(dtStart.date().dayOfWeek()-1);

        QTest::newRow("weekly nop") << false << dtStart << dtEnd << one_hour
                                    << 1 << KCalCore::RecurrenceRule::rWeekly
                                    << days << days << QDate() << QDate();
        //-------------------------------------------------------------------------
        // Test with multiple week days. Only the weekday from the old DTSTART should be unset.
        days.fill(true);
        expectedDays = days;
        expectedDays.clearBit(dtStart.date().dayOfWeek()-1);
        QTest::newRow("weekly multiple") << false << dtStart << dtEnd << one_day
                                         << 1 << KCalCore::RecurrenceRule::rWeekly
                                         << days << expectedDays << QDate() << QDate();
        //-------------------------------------------------------------------------
        // Testing moving an event such that DTSTART > recurrence end, which would
        // result in the event disappearing from all views.
        QTest::newRow("recur end") << false << dtStart << dtEnd << one_day*7
                                   << 1 << KCalCore::RecurrenceRule::rDaily
                                   << QBitArray() << QBitArray()
                                   << dtStart.date().addDays(3)
                                   << QDate();
        //-------------------------------------------------------------------------
        mCollection.setRights(Collection::Rights(Collection::AllRights));
    }

    void testAdjustRecurrence()
    {
        QFETCH(bool, allDay);
        QFETCH(KDateTime, dtStart);
        QFETCH(KDateTime, dtEnd);
        QFETCH(int, offsetToMove);
        QFETCH(int, frequency);
        QFETCH(KCalCore::RecurrenceRule::PeriodType, recurrenceType);
        QFETCH(QBitArray, weekDays);
        QFETCH(QBitArray, expectedWeekDays);
        QFETCH(QDate, recurrenceEnd);
        QFETCH(QDate, expectedRecurrenceEnd);

        Event::Ptr incidence = Event::Ptr(new Event());
        incidence->setSummary(QLatin1String("random summary"));
        incidence->setDtStart(dtStart);
        incidence->setDtEnd(dtEnd);
        incidence->setAllDay(allDay);

        Recurrence *recurrence = incidence->recurrence();

        switch (recurrenceType) {
        case KCalCore::RecurrenceRule::rDaily:
            recurrence->setDaily(frequency);
            break;
        case KCalCore::RecurrenceRule::rWeekly:
            recurrence->setWeekly(frequency, weekDays);
            break;
        default:
            // Not tested yet
            Q_ASSERT(false);
            QVERIFY(false);
        }

        if (recurrenceEnd.isValid()) {
            recurrence->setEndDate(recurrenceEnd);
        }

        // Create the recurring incidence
        int changeId = mChanger->createIncidence(incidence, mCollection);
        QVERIFY(changeId != -1);
        mIncidencesToAdd = 1;
        mExpectedResultByChangeId.insert(changeId, IncidenceChanger::ResultCodeSuccess);
        waitForSignals();

        // Now lets move it
        Incidence::Ptr originalIncidence = Incidence::Ptr(incidence->clone());
        incidence->setDtStart(dtStart.addSecs(offsetToMove));

        incidence->setDtEnd(dtEnd.addSecs(offsetToMove));
        mLastItemCreated.setPayload(incidence);
        changeId = mChanger->modifyIncidence(mLastItemCreated, originalIncidence);
        QVERIFY(changeId != -1);
        mIncidencesToModify = 1;
        mExpectedResultByChangeId.insert(changeId, IncidenceChanger::ResultCodeSuccess);
        waitForSignals();

        // Now check the results
        switch (recurrenceType) {
        case KCalCore::RecurrenceRule::rDaily:
            break;
        case KCalCore::RecurrenceRule::rWeekly:
            QCOMPARE(incidence->recurrence()->days(), expectedWeekDays);
            if (weekDays != expectedWeekDays) {
                QVERIFY(incidence->dirtyFields().contains(IncidenceBase::FieldRecurrence));
            }
            break;
        default:
            // Not tested yet
            Q_ASSERT(false);
            QVERIFY(false);
        }

        if (recurrenceEnd.isValid() && !expectedRecurrenceEnd.isValid()) {
            QVERIFY(incidence->recurrence()->endDate() >= incidence->dtStart().date());
            QVERIFY(incidence->dirtyFields().contains(IncidenceBase::FieldRecurrence));
        }
    }

public Q_SLOTS:

    void waitForSignals()
    {
        QTestEventLoop::instance().enterLoop(10);

        if (QTestEventLoop::instance().timeout()) {
            qDebug() << "Remaining: " << mIncidencesToAdd << mIncidencesToDelete << mIncidencesToModify;
        }
        QVERIFY(!QTestEventLoop::instance().timeout());
    }

    // Waits for a specific change
    void waitForChange(int changeId)
    {
        mChangeToWaitFor = changeId;

        int i = 0;
        while (mChangeToWaitFor != -1 && i++ < 10) {  // wait 10 seconds max.
            QTest::qWait(100);
        }

        QVERIFY(mChangeToWaitFor == -1);
    }

    void deleteFinished(int changeId,
                        const QVector<Akonadi::Item::Id> &deletedIds,
                        Akonadi::IncidenceChanger::ResultCode resultCode,
                        const QString &errorMessage)
    {
        QVERIFY(changeId != -1);
        mChangeToWaitFor = -1;
        --mIncidencesToDelete;

        if (resultCode != IncidenceChanger::ResultCodeSuccess) {
            kDebug() << "Error string is " << errorMessage;
        } else {
            QVERIFY(!deletedIds.isEmpty());
            foreach(Akonadi::Item::Id id , deletedIds) {
                QVERIFY(id != -1);
            }
        }

        compareExpectedResult(resultCode, mExpectedResultByChangeId[changeId],
                              QLatin1String("createFinished"));

        maybeQuitEventLoop();
    }

    void createFinished(int changeId,
                        const Akonadi::Item &item,
                        Akonadi::IncidenceChanger::ResultCode resultCode,
                        const QString &errorString)
    {
        QVERIFY(changeId != -1);
        mChangeToWaitFor = -1;
        --mIncidencesToAdd;

        if (resultCode == IncidenceChanger::ResultCodeSuccess) {
            QVERIFY(item.isValid());
            QVERIFY(item.parentCollection().isValid());
            mItemIdByChangeId.insert(changeId, item.id());
            QVERIFY(item.hasPayload());
            Incidence::Ptr incidence = item.payload<KCalCore::Incidence::Ptr>();
            mItemIdByUid.insert(incidence->uid(), item.id());
            mLastItemCreated = item;
        } else {
            kDebug() << "Error string is " << errorString;
        }

        compareExpectedResult(resultCode, mExpectedResultByChangeId[changeId],
                              QLatin1String("createFinished"));

        qDebug() << "Createfinished " << mIncidencesToAdd;
        maybeQuitEventLoop();
    }

    void modifyFinished(int changeId,
                        const Akonadi::Item &item,
                        Akonadi::IncidenceChanger::ResultCode resultCode,
                        const QString &errorString)
    {
        mChangeToWaitFor = -1;
        --mIncidencesToModify;
        QVERIFY(changeId != -1);

        if (resultCode == IncidenceChanger::ResultCodeSuccess)
            QVERIFY(item.isValid());
        else
            kDebug() << "Error string is " << errorString;

        compareExpectedResult(resultCode, mExpectedResultByChangeId[changeId],
                              QLatin1String("modifyFinished"));

        maybeQuitEventLoop();
    }

    void maybeQuitEventLoop()
    {
        if (mIncidencesToDelete == 0 && mIncidencesToAdd == 0 && mIncidencesToModify == 0)
            QTestEventLoop::instance().exitLoop();
    }

    void testDefaultCollection()
    {
        const Collection newCollection(42);
        IncidenceChanger changer;
        QCOMPARE(changer.defaultCollection(), Collection());
        changer.setDefaultCollection(newCollection);
        QCOMPARE(changer.defaultCollection(), newCollection);
    }

    void testDestinationPolicy()
    {
        IncidenceChanger changer;
        QCOMPARE(changer.destinationPolicy(), IncidenceChanger::DestinationPolicyDefault);
        changer.setDestinationPolicy(IncidenceChanger::DestinationPolicyNeverAsk);
        QCOMPARE(changer.destinationPolicy(), IncidenceChanger::DestinationPolicyNeverAsk);
    }

    void testDialogsOnError()
    {
        IncidenceChanger changer;
        QCOMPARE(changer.showDialogsOnError(), true);
        changer.setShowDialogsOnError(false);
        QCOMPARE(changer.showDialogsOnError(), false);
    }

    void testRespectsCollectionRights()
    {
        IncidenceChanger changer;
        QCOMPARE(changer.respectsCollectionRights(), true);
        changer.setRespectsCollectionRights(false);
        QCOMPARE(changer.respectsCollectionRights(), false);
    }

    void compareExpectedResult(IncidenceChanger::ResultCode result,
                               IncidenceChanger::ResultCode expected, const QLatin1String &str)
    {
        if (mPermissionsOrRollback) {
            if (expected == IncidenceChanger::ResultCodePermissions)
                expected = IncidenceChanger::ResultCodeRolledback;

            if (result == IncidenceChanger::ResultCodePermissions)
                result = IncidenceChanger::ResultCodeRolledback;
        }

        if (mDiscardedEqualsSuccess) {
            if (expected == IncidenceChanger::ResultCodeModificationDiscarded)
                expected = IncidenceChanger::ResultCodeSuccess;

            if (result == IncidenceChanger::ResultCodeModificationDiscarded)
                result = IncidenceChanger::ResultCodeSuccess;
        }

        if (result != expected) {
            qDebug() << str << "Expected " << expected << " got " << result;
        }

        QCOMPARE(result, expected);
    }
};

QTEST_AKONADIMAIN(IncidenceChangerTest, GUI)

#include "incidencechangertest.moc"
