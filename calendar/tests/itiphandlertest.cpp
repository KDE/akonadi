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
Q_DECLARE_METATYPE(Akonadi::ITIPHandler::Result)

static const char *s_ourEmail = "kdepimlibs-unittests@kde.org";


void ITIPHandlerTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();
    m_itipHandler = new Akonadi::ITIPHandler(this);
    m_itipHandler->setShowDialogsOnError(false);
    connect(m_itipHandler, SIGNAL(iTipMessageProcessed(Akonadi::ITIPHandler::Result,QString)),
            SLOT(oniTipMessageProcessed(Akonadi::ITIPHandler::Result,QString)) );
    m_pendingItipMessageSignal = 0;
}

void ITIPHandlerTest::testProcessITIPMessage_data()
{
    QTest::addColumn<QString>("data_filename");
    QTest::addColumn<QString>("action");
    QTest::addColumn<QString>("receiver");
    QTest::addColumn<Akonadi::ITIPHandler::Result>("expectedResult");

    QString action = QLatin1String("accepted");
    QString receiver = QLatin1String(s_ourEmail);
    Akonadi::ITIPHandler::Result expectedResult;

    //----------------------------------------------------------------------------------------------
    // Here we're testing an error case, where data is null.
    expectedResult = ITIPHandler::ResultError;
    QTest::newRow("invalid data") << QString() << action << receiver << expectedResult;

    //----------------------------------------------------------------------------------------------
    expectedResult = ITIPHandler::ResultError;
    QTest::newRow("invalid receiver") << QString() << action << QString() << expectedResult;

    //----------------------------------------------------------------------------------------------

    //----------------------------------------------------------------------------------------------
}

void ITIPHandlerTest::testProcessITIPMessage()
{
    QFETCH(QString, data_filename);
    QFETCH(QString, action);
    QFETCH(QString, receiver);
    QFETCH(Akonadi::ITIPHandler::Result, expectedResult);

    m_expectedResult = expectedResult;

    QString iCalData = QString::fromLatin1(readFile(data_filename));

    m_pendingItipMessageSignal = 1;
    m_itipHandler->processiTIPMessage(receiver, iCalData, action);
    waitForIt();
}

void ITIPHandlerTest::oniTipMessageProcessed(ITIPHandler::Result result, const QString &errorMessage)
{
    if (result != ITIPHandler::ResultSuccess && result != m_expectedResult) {
        qDebug() << "ITIPHandlerTest::oniTipMessageProcessed() error = " << errorMessage;
    }

    QCOMPARE(m_expectedResult, result);

    m_pendingItipMessageSignal--;
    QVERIFY(m_pendingItipMessageSignal >= 0);
    if (m_pendingItipMessageSignal == 0) {
        stopWaiting();
    }
}

QTEST_AKONADIMAIN(ITIPHandlerTest, GUI)
