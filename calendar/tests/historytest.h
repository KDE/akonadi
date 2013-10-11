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

#ifndef HISTORY_TEST_H
#define HISTORY_TEST_H

#include "unittestbase.h"
#include "../history.h"
#include "../history_p.h"
#include "../incidencechanger.h"

enum SignalType {
    DeletionSignal,
    CreationSignal,
    ModificationSignal,
    UndoSignal,
    RedoSignal,
    NumSignals
};

class HistoryTest : public UnitTestBase
{
    Q_OBJECT
    History *mHistory;
    QHash<SignalType, int> mPendingSignals;
    QHash<int, Akonadi::Item> mItemByChangeId;
    QList<int> mKnownChangeIds;

    void createIncidence(const QString &uid);

private Q_SLOTS:
    void initTestCase();

    void testCreation_data();
    void testCreation();

    void testDeletion_data();
    void testDeletion();

    void testModification_data();
    void testModification();

    void testAtomicOperations_data();
    void testAtomicOperations();

    // Tests a sequence of various create/delete/modify operations that are not part
    // of an atomic-operation.
    void testMix_data();
    void testMix();

private:
    void waitForSignals();

public Q_SLOTS:
    void deleteFinished(int changeId,
                        const QVector<Akonadi::Item::Id> &deletedIds,
                        Akonadi::IncidenceChanger::ResultCode resultCode,
                        const QString &errorMessage);

    void createFinished(int changeId,
                        const Akonadi::Item &item,
                        Akonadi::IncidenceChanger::ResultCode resultCode,
                        const QString &errorString);

    void modifyFinished(int changeId,
                        const Akonadi::Item &item,
                        Akonadi::IncidenceChanger::ResultCode resultCode,
                        const QString &errorString);

    void handleRedone(Akonadi::History::ResultCode result);
    void handleUndone(Akonadi::History::ResultCode result);
    void maybeQuitEventLoop();
};

#endif
