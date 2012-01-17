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

#define MAILCLIENTTEST_UNITTEST
#include "../mailclient_p.cpp"
#include "../moc_mailclient_p.cpp"

#include <KCalCore/Incidence>
#include <Mailtransport/MessageQueueJob>


#include <QTestEventLoop>
#include <akonadi/qtest_akonadi.h>

#include <QtCore/QObject>

using namespace Akonadi;

Q_DECLARE_METATYPE( KPIMIdentities::Identity );
Q_DECLARE_METATYPE( KCalCore::Incidence::Ptr );

class MailClientTest : public QObject
{
  Q_OBJECT

private:
  MailClient *mMailClient;
  int mPendingSignals;
  MailClient::Result mLastResult;

private slots:

  void initTestCase()
  {
    mPendingSignals = 0;
    mMailClient = new MailClient( this );
    mLastResult = MailClient::ResultSuccess;
    connect( mMailClient, SIGNAL(finished(Akonadi::MailClient::Result,QString)),
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

    KCalCore::Incidence::Ptr incidence( new KCalCore::Event() );
    KPIMIdentities::Identity identity;
    bool bccMe;
    QString attachment;
    QString transport;
    MailClient::Result expectedResult = MailClient::ResultNoAttendees;

    //----------------------------------------------------------------------------------------------
    QTest::newRow("No attendees") << incidence << identity << bccMe << attachment << transport
                                  << expectedResult;
    //----------------------------------------------------------------------------------------------
    // One attendee, but without e-mail
    KCalCore::Attendee::Ptr attendee( new KCalCore::Attendee( QLatin1String( "name1" ),
                                                              QString() ) );
    incidence = KCalCore::Incidence::Ptr( new KCalCore::Event() );
    incidence->addAttendee( attendee );
    expectedResult = MailClient::ResultReallyNoAttendees;
    QTest::newRow("No attendees with email") << incidence << identity << bccMe << attachment << transport
                                             << expectedResult;
    //----------------------------------------------------------------------------------------------
    // One valid attendee
    attendee = KCalCore::Attendee::Ptr ( new KCalCore::Attendee( QLatin1String( "name1" ),
                                                                 QLatin1String( "test@foo.org" ) ) );
    incidence = KCalCore::Incidence::Ptr( new KCalCore::Event() );
    incidence->addAttendee( attendee );
    expectedResult = MailClient::ResultReallyNoAttendees;
    QTest::newRow("On attendees") << incidence << identity << bccMe << attachment << transport
                                  << expectedResult;
  }

  void testMailAttendees()
  {
    QFETCH( KCalCore::Incidence::Ptr, incidence );
    QFETCH( KPIMIdentities::Identity, identity );
    QFETCH( bool, bccMe );
    QFETCH( QString, attachment );
    QFETCH( QString, transport );
    QFETCH( MailClient::Result, expectedResult );

    mPendingSignals = 1;
    mMailClient->mailAttendees( incidence, identity, bccMe, attachment, transport );
    waitForSignals();
    QCOMPARE( mLastResult, expectedResult );
  }

  void testMailOrganizer()
  {
  }

  void testMailTo()
  {

  }

  void handleFinished( Akonadi::MailClient::Result result, const QString &errorMessage )
  {
    kDebug() << "handleFinished: " << result << errorMessage;
    mLastResult = result;
    --mPendingSignals;
    QTestEventLoop::instance().exitLoop();
  }

  void waitForSignals()
  {
    if ( mPendingSignals > 0 ) {
      QTestEventLoop::instance().enterLoop( 5 ); // 5 seconds is enough
    }
  }


public Q_SLOTS:
private:

};

QTEST_AKONADIMAIN( MailClientTest, GUI )

#include "mailclienttest.moc"
