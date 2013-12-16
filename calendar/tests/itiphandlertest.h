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

#ifndef ITIPHANDLER_TEST_H
#define ITIPHANDLER_TEST_H

#include "../incidencechanger.h"
#include "../itiphandler.h"
#include "unittestbase.h"

#include <akonadi/collection.h>
#include <akonadi/item.h>

#include <QObject>
#include <QHash>


class ITIPHandlerTest : public UnitTestBase
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void testProcessITIPMessages_data();
    void testProcessITIPMessages();

    // Deprecated methods, use testProcessITIPMessages() for new stuff
    void testProcessITIPMessage_data();
    void testProcessITIPMessage();

    // Deprecated methods do test CANCEL.
    void testProcessITIPMessageCancel_data();
    void testProcessITIPMessageCancel();

    // These ones don't have to do with kmail. It's when doing a modification, itip REQUESTs are sent.
    // Also tests cases where we're not the organizer.
    void testOutgoingInvitations_data();
    void testOutgoingInvitations();

    // Tests identity related code, like "thisIsMe()".
    void testIdentity_data();
    void testIdentity();

private:
    void waitForSignals();
    void cleanup();
    void createITIPHandler();
    QString icalData(const QString &filename);
    void processItip(const QString &icaldata, const QString &receiver,
                     const QString &action, int expectedNumIncidences,
                     Akonadi::Item::List &items);
    KCalCore::Attendee::Ptr ourAttendee(const KCalCore::Incidence::Ptr &incidence) const;

public Q_SLOTS:
    void oniTipMessageProcessed(Akonadi::ITIPHandler::Result result,
                                const QString &errorMessage);

    void onCreateFinished(int changeId, const Akonadi::Item &item,
                          Akonadi::IncidenceChanger::ResultCode resultCode,
                          const QString &errorString);

    void onDeleteFinished(int changeId, const QVector<Akonadi::Item::Id> &deletedIds,
                          Akonadi::IncidenceChanger::ResultCode resultCode,
                          const QString &errorMessage);

    void onModifyFinished(int changeId, const Akonadi::Item &item,
                          Akonadi::IncidenceChanger::ResultCode resultCode,
                          const QString &errorString);

private:
    int m_pendingItipMessageSignal;
    int m_pendingIncidenceChangerSignal;
    Akonadi::Item mLastInsertedItem;
    Akonadi::ITIPHandler::Result m_expectedResult;
    Akonadi::ITIPHandler *m_itipHandler;
    Akonadi::IncidenceChanger *m_changer;
    bool m_cancelExpected;
};

#endif
