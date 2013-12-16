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

#include "historytest.h"
#include "helper.h"

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/qtest_akonadi.h>
#include <kcalcore/event.h>

#include <QTestEventLoop>

using namespace Akonadi;
using namespace KCalCore;

Q_DECLARE_METATYPE(QList<Akonadi::IncidenceChanger::ChangeType>)


static bool checkSummary(const Akonadi::Item &item, const QString &expected)
{
    ItemFetchJob *job = new ItemFetchJob(item);
    job->fetchScope().fetchFullPayload();
    Q_ASSERT(job->exec());
    Q_ASSERT(!job->items().isEmpty());
    Item it = job->items().first();
    Q_ASSERT(it.hasPayload());

    if (it.payload<KCalCore::Incidence::Ptr>()->summary() == expected) {
        return true;
    } else {
        qDebug() << "Got " << it.payload<KCalCore::Incidence::Ptr>()->summary()
                 << "Expected " << expected;
        return false;
    }
}

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

    Q_ASSERT(createJob->exec());
    Q_ASSERT(createJob->item().isValid());
    return createJob->item();
}

void HistoryTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();

    mHistory = mChanger->history();

    connect(mChanger,
             SIGNAL(createFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
             SLOT(createFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)));

    connect(mChanger,
             SIGNAL(deleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)),
             SLOT(deleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)));

    connect(mChanger,
             SIGNAL(modifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
             SLOT(modifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)));

    connect(mHistory, SIGNAL(undone(Akonadi::History::ResultCode)),
             SLOT(handleUndone(Akonadi::History::ResultCode)));

    connect(mHistory, SIGNAL(redone(Akonadi::History::ResultCode)),
             SLOT(handleRedone(Akonadi::History::ResultCode)));
}

void HistoryTest::testCreation_data()
{
    QTest::addColumn<Akonadi::Item>("item");
    QTest::newRow("item1") << item();
}

void HistoryTest::testCreation()
{
    QFETCH(Akonadi::Item, item);
    mPendingSignals[CreationSignal] = 1;
    QCOMPARE(mHistory->d->redoCount(), 0);
    QCOMPARE(mHistory->d->undoCount(), 0);
    QVERIFY(item.hasPayload());
    const int changeId = mChanger->createIncidence(item.payload<KCalCore::Incidence::Ptr>());
    QVERIFY(changeId > 0);
    mKnownChangeIds << changeId;
    waitForSignals();

    // Check that it was created
    QVERIFY(Helper::confirmExists(mItemByChangeId.value(changeId)));

    QCOMPARE(mHistory->d->redoCount(), 0);
    QCOMPARE(mHistory->d->undoCount(), 1);

    //undo it
    mPendingSignals[UndoSignal] = 1;
    mHistory->undo();
    waitForSignals();

    // Check that it doesn't exist anymore
    QVERIFY(Helper::confirmDoesntExist(mItemByChangeId.value(changeId)));

    QCOMPARE(mHistory->d->redoCount(), 1);
    QCOMPARE(mHistory->d->undoCount(), 0);

    mPendingSignals[RedoSignal] = 1;
    mHistory->redo();
    waitForSignals();

    // Undo again just for fun
    mPendingSignals[UndoSignal] = 1;
    mHistory->undo();
    waitForSignals();

    mHistory->clear();
    QCOMPARE(mHistory->d->redoCount(), 0);
    QCOMPARE(mHistory->d->undoCount(), 0);
}

void HistoryTest::testDeletion_data()
{
    QTest::addColumn<Akonadi::Item::List>("items");
    Akonadi::Item::List items;
    items << createItem(mCollection);
    QTest::newRow("one item") << items;

    items.clear();
    items << createItem(mCollection) << createItem(mCollection);
    QTest::newRow("two items") << items;

    items.clear();
    items << createItem(mCollection) << createItem(mCollection) << createItem(mCollection)
          << createItem(mCollection);
    QTest::newRow("four items") << items;
}

void HistoryTest::testDeletion()
{
    QFETCH(Akonadi::Item::List, items);
    mPendingSignals[DeletionSignal] = 1;
    QCOMPARE(mHistory->d->redoCount(), 0);
    QCOMPARE(mHistory->d->undoCount(), 0);

    const int changeId = (items.count() == 1) ? mChanger->deleteIncidence(items.first())
                                              : mChanger->deleteIncidences(items);
    QVERIFY(changeId > 0);
    mKnownChangeIds << changeId;
    waitForSignals();

    // Check that it doesn't exist anymore
    foreach(const Akonadi::Item &item, items) {
        QVERIFY(Helper::confirmDoesntExist(item));
    }

    mPendingSignals[UndoSignal] = 1;
    mHistory->undo();
    waitForSignals();

    mPendingSignals[RedoSignal] = 1;
    mHistory->redo();
    waitForSignals();

    mHistory->clear();
    QCOMPARE(mHistory->d->redoCount(), 0);
    QCOMPARE(mHistory->d->undoCount(), 0);
}

void HistoryTest::testModification_data()
{
    QTest::addColumn<Akonadi::Item>("item");
    QTest::addColumn<QString>("oldSummary");
    QTest::addColumn<QString>("newSummary");
    Item item1 = createItem(mCollection);
    const QString oldSummary(QLatin1String("old"));
    const QString newSummary(QLatin1String("new"));
    item1.payload<KCalCore::Incidence::Ptr>()->setSummary(oldSummary);
    QTest::newRow("item1") << item1 << oldSummary << newSummary;
}

void HistoryTest::testModification()
{
    QFETCH(Akonadi::Item, item);
    QFETCH(QString, oldSummary);
    QFETCH(QString, newSummary);
    QVERIFY(item.hasPayload());
    Incidence::Ptr originalPayload = Incidence::Ptr(item.payload<KCalCore::Incidence::Ptr>()->clone());

    item.payload<KCalCore::Incidence::Ptr>()->setSummary(newSummary);
    mPendingSignals[ModificationSignal] = 1;
    QCOMPARE(mHistory->d->redoCount(), 0);
    QCOMPARE(mHistory->d->undoCount(), 0);

    const int changeId = mChanger->modifyIncidence(item, originalPayload);
    QVERIFY(changeId > 0);
    mKnownChangeIds << changeId;
    waitForSignals();

    QVERIFY(checkSummary(item, newSummary));
    QCOMPARE(mHistory->d->redoCount(), 0);
    QCOMPARE(mHistory->d->undoCount(), 1);

    mPendingSignals[UndoSignal] = 1;
    mHistory->undo();
    waitForSignals();
    QVERIFY(checkSummary(item, oldSummary));
    QCOMPARE(mHistory->d->redoCount(), 1);
    QCOMPARE(mHistory->d->undoCount(), 0);

    mPendingSignals[RedoSignal] = 1;
    mHistory->redo();
    waitForSignals();
    QVERIFY(checkSummary(item, newSummary));
    QCOMPARE(mHistory->d->redoCount(), 0);
    QCOMPARE(mHistory->d->undoCount(), 1);

    mHistory->clear();
    QCOMPARE(mHistory->d->redoCount(), 0);
    QCOMPARE(mHistory->d->undoCount(), 0);

    // Test that it isn't recorded to history when history is disabled
    mChanger->setHistoryEnabled(false);
    mPendingSignals[ModificationSignal] = 1;
    const int changeId2 = mChanger->modifyIncidence(item, originalPayload);
    mChanger->setHistoryEnabled(true);
    QVERIFY(changeId > 0);
    mKnownChangeIds << changeId2;
    waitForSignals();
    QCOMPARE(mHistory->d->redoCount(), 0);
    QCOMPARE(mHistory->d->undoCount(), 0);
}

void HistoryTest::testAtomicOperations_data()
{
    QTest::addColumn<Akonadi::Item::List>("items");
    QTest::addColumn<QList<Akonadi::IncidenceChanger::ChangeType> >("changeTypes");

    Akonadi::Item::List items;
    QList<Akonadi::IncidenceChanger::ChangeType> changeTypes;
    //------------------------------------------------------------------------------------------
    // Create two incidences, should succeed
    items << item() << item();
    changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeCreate;

    QTest::newRow("create two - success ") << items << changeTypes;
    //------------------------------------------------------------------------------------------
    changeTypes.clear();
    changeTypes << IncidenceChanger::ChangeTypeModify << IncidenceChanger::ChangeTypeModify;
    items.clear();
    items << createItem(mCollection) << createItem(mCollection);

    QTest::newRow("modify two - success ") << items << changeTypes;
    //------------------------------------------------------------------------------------------
    changeTypes.clear();
    changeTypes << IncidenceChanger::ChangeTypeDelete << IncidenceChanger::ChangeTypeDelete;
    QTest::newRow("delete two - success ") << items << changeTypes;
}

void HistoryTest::testAtomicOperations()
{
    mHistory->clear();
    QFETCH(Akonadi::Item::List, items);
    QFETCH(QList<Akonadi::IncidenceChanger::ChangeType>, changeTypes);
    mChanger->setDefaultCollection(mCollection);
    mChanger->setRespectsCollectionRights(false);
    mChanger->setDestinationPolicy(IncidenceChanger::DestinationPolicyNeverAsk);
    mChanger->startAtomicOperation();

    for (int i=0; i<items.count(); ++i) {
        const Akonadi::Item item = items[i];
        int changeId = -1;
        switch(changeTypes[i]) {
        case IncidenceChanger::ChangeTypeCreate:
            changeId = mChanger->createIncidence(item.hasPayload() ? item.payload<KCalCore::Incidence::Ptr>()
                                                                   : Incidence::Ptr());
            QVERIFY(changeId != -1);
            mKnownChangeIds << changeId;
            ++mPendingSignals[CreationSignal];
            break;
        case IncidenceChanger::ChangeTypeDelete:
            changeId = mChanger->deleteIncidence(item);
            QVERIFY(changeId != -1);
            mKnownChangeIds << changeId;
            ++mPendingSignals[DeletionSignal];
            break;
        case IncidenceChanger::ChangeTypeModify:
        {
            QVERIFY(item.isValid());
            QVERIFY(item.hasPayload<KCalCore::Incidence::Ptr>());
            Incidence::Ptr originalPayload = Incidence::Ptr(item.payload<KCalCore::Incidence::Ptr>()->clone());
            item.payload<KCalCore::Incidence::Ptr>()->setSummary(QLatin1String("Changed"));
            changeId = mChanger->modifyIncidence(item, originalPayload);
            QVERIFY(changeId != -1);
            mKnownChangeIds << changeId;
            ++mPendingSignals[ModificationSignal];
        }
            break;
        default:
            QVERIFY(false);
        }
    }

    mChanger->endAtomicOperation();
    waitForSignals();

    QCOMPARE(mHistory->d->undoCount(), 1);
    QCOMPARE(mHistory->d->redoCount(), 0);

    mPendingSignals[UndoSignal] = 1;
    mHistory->undo();
    waitForSignals();
    QCOMPARE(mHistory->d->undoCount(), 0);
    QCOMPARE(mHistory->d->redoCount(), 1);

    // Verify that it got undone
    for (int i=0; i<items.count(); ++i) {
        const Akonadi::Item item = items[i];
        switch(changeTypes[i]) {
        case IncidenceChanger::ChangeTypeCreate:
            // It changed id, have no way to verify
            break;
        case IncidenceChanger::ChangeTypeDelete:
            QVERIFY(Helper::confirmDoesntExist(item));
            break;
        case IncidenceChanger::ChangeTypeModify:
            QVERIFY(checkSummary(item, QLatin1String("random summary")));
            break;
        default:
            QVERIFY(false);
        }
    }

    mPendingSignals[RedoSignal] = 1;
    mHistory->redo();
    waitForSignals();
    QCOMPARE(mHistory->d->undoCount(), 1);
    QCOMPARE(mHistory->d->redoCount(), 0);

    mPendingSignals[UndoSignal] = 1;
    mHistory->undo();
    waitForSignals();
    QCOMPARE(mHistory->d->undoCount(), 0);
    QCOMPARE(mHistory->d->redoCount(), 1);

    mHistory->clear();
    QCOMPARE(mHistory->d->redoCount(), 0);
    QCOMPARE(mHistory->d->undoCount(), 0);
}


// Tests a sequence of various create/delete/modify operations
void HistoryTest::testMix_data()
{
    QTest::addColumn<Akonadi::Item::List>("items");
    QTest::addColumn<QList<Akonadi::IncidenceChanger::ChangeType> >("changeTypes");

    Akonadi::Item::List items;
    QList<Akonadi::IncidenceChanger::ChangeType> changeTypes;
    //------------------------------------------------------------------------------------------
    // Create two incidences
    items.clear();
    items << item() << item();
    changeTypes.clear();
    changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeCreate;
    QTest::newRow("create two - success ") << items << changeTypes;
    //------------------------------------------------------------------------------------------
    // Create one, then delete it
    Akonadi::Item i = item();
    items.clear();
    items << i << i;
    changeTypes.clear();
    changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeDelete;
    QTest::newRow("create then delete") << items << changeTypes;
    //------------------------------------------------------------------------------------------
    // Create one, then modify it, then delete it
    i = item();
    items.clear();
    items << i << i << i;
    changeTypes.clear();
    changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeModify << IncidenceChanger::ChangeTypeDelete;
    QTest::newRow("create then modify then deelte") << items << changeTypes;
}

void HistoryTest::testMix()
{
    QFETCH(Akonadi::Item::List, items);
    QFETCH(QList<Akonadi::IncidenceChanger::ChangeType>, changeTypes);
    int lastCreateChangeId = -1;
    mHistory->clear();
    mChanger->setDefaultCollection(mCollection);
    mChanger->setRespectsCollectionRights(false);
    mChanger->setDestinationPolicy(IncidenceChanger::DestinationPolicyNeverAsk);

    for (int i=0; i<items.count(); ++i) {
        Akonadi::Item item = items[i];
        int changeId = -1;
        switch (changeTypes[i]) {
        case IncidenceChanger::ChangeTypeCreate:
            lastCreateChangeId = mChanger->createIncidence(item.payload<KCalCore::Incidence::Ptr>());
            QVERIFY(lastCreateChangeId != -1);
            mKnownChangeIds << lastCreateChangeId;
            ++mPendingSignals[CreationSignal];
            waitForSignals();
            break;
        case IncidenceChanger::ChangeTypeDelete:
            item = item.isValid() ? item : mItemByChangeId.value(lastCreateChangeId);
            QVERIFY(item.isValid());
            changeId = mChanger->deleteIncidence(item);
            QVERIFY(changeId != -1);
            mKnownChangeIds << changeId;
            ++mPendingSignals[DeletionSignal];
            waitForSignals();
            break;
        case IncidenceChanger::ChangeTypeModify:
        {
            item = item.isValid() ? item : mItemByChangeId.value(lastCreateChangeId);
            QVERIFY(item.isValid());
            QVERIFY(item.hasPayload<KCalCore::Incidence::Ptr>());
            Incidence::Ptr originalPayload = Incidence::Ptr(item.payload<KCalCore::Incidence::Ptr>()->clone());
            item.payload<KCalCore::Incidence::Ptr>()->setSummary(QLatin1String("Changed"));
            QVERIFY(originalPayload);
            changeId = mChanger->modifyIncidence(item, originalPayload);
            QVERIFY(changeId != -1);
            mKnownChangeIds << changeId;
            ++mPendingSignals[ModificationSignal];
            waitForSignals();
        }
            break;
        default:
            QVERIFY(false);
        }

    }

    QCOMPARE(mHistory->d->undoCount(), changeTypes.count());

    // All operations are done, now undo them:
    for (int i=0; i<changeTypes.count(); i++) {
        QCOMPARE(mHistory->d->undoCount(), changeTypes.count()-i);
        QCOMPARE(mHistory->d->redoCount(), i);
        mPendingSignals[UndoSignal] = 1;
        mHistory->undo();
        waitForSignals();
    }

    QCOMPARE(mHistory->d->undoCount(), 0);
    QCOMPARE(mHistory->d->redoCount(), changeTypes.count());

    // Now redo them
    for (int i=0; i<changeTypes.count(); i++) {
        QCOMPARE(mHistory->d->undoCount(), i);
        QCOMPARE(mHistory->d->redoCount(), changeTypes.count()-i);
        mPendingSignals[RedoSignal] = 1;
        mHistory->redo();
        waitForSignals();
    }

    QCOMPARE(mHistory->d->undoCount(), changeTypes.count());
    QCOMPARE(mHistory->d->redoCount(), 0);
}


void HistoryTest::waitForSignals()
{
    bool somethingToWaitFor = false;
    for (int i=0; i<NumSignals; ++i) {
        if (mPendingSignals.value(static_cast<SignalType>(i))) {
            somethingToWaitFor = true;
            break;
        }
    }

    if (!somethingToWaitFor)
        return;

    QTestEventLoop::instance().enterLoop(10);

    if (QTestEventLoop::instance().timeout()) {
        for (int i=0; i<NumSignals; ++i) {
            qDebug() << mPendingSignals.value(static_cast<SignalType>(i));
        }
    }

    QVERIFY(!QTestEventLoop::instance().timeout());
}


void HistoryTest::deleteFinished(int changeId,
                                 const QVector<Akonadi::Item::Id> &deletedIds,
                                 Akonadi::IncidenceChanger::ResultCode resultCode,
                                 const QString &errorMessage)
{
    QVERIFY(changeId != -1);

    if (!mKnownChangeIds.contains(changeId))
        return;

    QVERIFY(mPendingSignals[DeletionSignal] > 0);
    --mPendingSignals[DeletionSignal];

    if (resultCode != IncidenceChanger::ResultCodeSuccess) {
        kDebug() << "Error string is " << errorMessage;
    } else {
        QVERIFY(!deletedIds.isEmpty());
        foreach(Akonadi::Item::Id id , deletedIds) {
            QVERIFY(id != -1);
        }
    }
    maybeQuitEventLoop();
}

void HistoryTest::createFinished(int changeId,
                                  const Akonadi::Item &item,
                                  Akonadi::IncidenceChanger::ResultCode resultCode,
                                  const QString &errorString)
{
    QVERIFY(changeId != -1);

    if (!mKnownChangeIds.contains(changeId))
        return;

    QVERIFY(mPendingSignals[CreationSignal] > 0);

    --mPendingSignals[CreationSignal];

    if (resultCode == IncidenceChanger::ResultCodeSuccess) {
        QVERIFY(item.isValid());
        QVERIFY(item.parentCollection().isValid());
        mItemByChangeId.insert(changeId, item);
        QVERIFY(item.hasPayload());
        Incidence::Ptr incidence = item.payload<KCalCore::Incidence::Ptr>();
        //mItemIdByUid.insert(incidence->uid(), item.id());
    } else {
        kDebug() << "Error string is " << errorString;
    }

    maybeQuitEventLoop();
}

void HistoryTest::modifyFinished(int changeId,
                                 const Akonadi::Item &item,
                                 Akonadi::IncidenceChanger::ResultCode resultCode,
                                 const QString &errorString)
{
    if (!mKnownChangeIds.contains(changeId))
        return;

    QVERIFY(mPendingSignals[ModificationSignal] > 0);
    --mPendingSignals[ModificationSignal];

    QVERIFY(changeId != -1);
    QCOMPARE(resultCode, IncidenceChanger::ResultCodeSuccess);

    if (resultCode == IncidenceChanger::ResultCodeSuccess)
        QVERIFY(item.isValid());
    else
        kDebug() << "Error string is " << errorString;

    maybeQuitEventLoop();
}

void HistoryTest::handleRedone(Akonadi::History::ResultCode result)
{
    QCOMPARE(result, History::ResultCodeSuccess);
    --mPendingSignals[RedoSignal];
    maybeQuitEventLoop();
}

void HistoryTest::handleUndone(Akonadi::History::ResultCode result)
{
    QCOMPARE(result, History::ResultCodeSuccess);
    --mPendingSignals[UndoSignal];
    maybeQuitEventLoop();
}

void HistoryTest::maybeQuitEventLoop()
{
    for (int i=0; i<NumSignals; ++i) {
        if (mPendingSignals.value(static_cast<SignalType>(i)) > 0)
            return;
    }
    QTestEventLoop::instance().exitLoop();
}


QTEST_AKONADIMAIN(HistoryTest, GUI)
