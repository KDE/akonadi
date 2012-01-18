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
#include <KCalCore/FreeBusy>
#include <Mailtransport/MessageQueueJob>


#include <QTestEventLoop>
#include <akonadi/qtest_akonadi.h>

#include <QtCore/QObject>

using namespace Akonadi;

Q_DECLARE_METATYPE( KPIMIdentities::Identity );
Q_DECLARE_METATYPE( KCalCore::Incidence::Ptr );
Q_DECLARE_METATYPE( KCalCore::IncidenceBase::Ptr );

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
    QTest::addColumn<int>("expectedTransportId");
    QTest::addColumn<QString>("expectedFrom");
    QTest::addColumn<QStringList>("expectedToList");
    QTest::addColumn<QStringList>("expectedCcList");
    QTest::addColumn<QStringList>("expectedBccList");

    KCalCore::Incidence::Ptr incidence( new KCalCore::Event() );
    KPIMIdentities::Identity identity;
    bool bccMe;
    QString attachment;
    QString transport;
    MailClient::Result expectedResult = MailClient::ResultNoAttendees;
    const int expectedTransportId = 69372773; // from tests/unittestenv/kdehome/share/config/mailtransports
    const QString expectedFrom = QLatin1String( "unittests@dev.nul" ); // from tests/unittestenv/kdehome/share/config/emailidentities
    KCalCore::Person::Ptr organizer( new KCalCore::Person( QLatin1String( "Organizer" ),
                                                           QLatin1String( "unittests@dev.nul" ) ) );

    QStringList toList;
    QStringList toCcList;
    QStringList toBccList;
    //----------------------------------------------------------------------------------------------
    QTest::newRow("No attendees") << incidence << identity << bccMe << attachment << transport
                                  << expectedResult << -1 << QString()
                                  << toList << toCcList << toBccList;
    //----------------------------------------------------------------------------------------------
    // One attendee, but without e-mail
    KCalCore::Attendee::Ptr attendee( new KCalCore::Attendee( QLatin1String( "name1" ),
                                                              QString() ) );
    incidence = KCalCore::Incidence::Ptr( new KCalCore::Event() );
    incidence->addAttendee( attendee );
    expectedResult = MailClient::ResultReallyNoAttendees;
    QTest::newRow("No attendees with email") << incidence << identity << bccMe << attachment << transport
                                             << expectedResult << -1 << QString()
                                             << toList << toCcList << toBccList;
    //----------------------------------------------------------------------------------------------
    // One valid attendee
    attendee = KCalCore::Attendee::Ptr ( new KCalCore::Attendee( QLatin1String( "name1" ),
                                                                 QLatin1String( "test@foo.org" ) ) );
    incidence = KCalCore::Incidence::Ptr( new KCalCore::Event() );
    incidence->addAttendee( attendee );
    incidence->setOrganizer( organizer );
    expectedResult = MailClient::ResultSuccess;
    toList << QLatin1String( "name1 <test@foo.org>" );
    QTest::newRow("One attendee") << incidence << identity << bccMe << attachment << transport
                                  << expectedResult << expectedTransportId << expectedFrom
                                  << toList << toCcList << toBccList;
    //----------------------------------------------------------------------------------------------
    // One valid attendee
    attendee = KCalCore::Attendee::Ptr ( new KCalCore::Attendee( QLatin1String( "name1" ),
                                                                 QLatin1String( "test@foo.org" ) ) );
    incidence = KCalCore::Incidence::Ptr( new KCalCore::Event() );
    incidence->setOrganizer( organizer );
    incidence->addAttendee( attendee );
    QString invalidTransport = QLatin1String( "foo" );
    expectedResult = MailClient::ResultSuccess;
    // Should default to the default transport
    QTest::newRow("Invalid transport") << incidence << identity << bccMe << attachment
                                       << invalidTransport  << expectedResult
                                       << expectedTransportId << expectedFrom
                                       << toList << toCcList << toBccList;
    //----------------------------------------------------------------------------------------------
    // One valid attendee, and bcc me
    attendee = KCalCore::Attendee::Ptr ( new KCalCore::Attendee( QLatin1String( "name1" ),
                                                                 QLatin1String( "test@foo.org" ) ) );
    incidence = KCalCore::Incidence::Ptr( new KCalCore::Event() );
    incidence->setOrganizer( organizer );
    incidence->addAttendee( attendee );
    expectedResult = MailClient::ResultSuccess;
    // Should default to the default transport
    toBccList.clear();
    toBccList << QLatin1String( "Organizer <unittests@dev.nul>" );
    QTest::newRow("Invalid transport") << incidence << identity << /*bccMe*/true << attachment
                                       << transport  << expectedResult
                                       << expectedTransportId << expectedFrom
                                       << toList << toCcList << toBccList;
    //----------------------------------------------------------------------------------------------
    // Test CC list
    attendee = KCalCore::Attendee::Ptr ( new KCalCore::Attendee( QLatin1String( "name1" ),
                                                                 QLatin1String( "test@foo.org" ) ) );
    KCalCore::Attendee::Ptr optionalAttendee =
                            KCalCore::Attendee::Ptr( new KCalCore::Attendee( QLatin1String( "opt" ),
                                                     QLatin1String( "optional@foo.org" ) ) );
    KCalCore::Attendee::Ptr nonParticipant =
                            KCalCore::Attendee::Ptr( new KCalCore::Attendee( QLatin1String( "non" ),
                                                     QLatin1String( "non@foo.org" ) ) );
    optionalAttendee->setRole( KCalCore::Attendee::OptParticipant );
    nonParticipant->setRole( KCalCore::Attendee::NonParticipant );
    incidence = KCalCore::Incidence::Ptr( new KCalCore::Event() );
    incidence->setOrganizer( organizer );
    incidence->addAttendee( attendee );
    incidence->addAttendee( optionalAttendee );
    incidence->addAttendee( nonParticipant );
    expectedResult = MailClient::ResultSuccess;
    // Should default to the default transport
    toBccList.clear();
    toBccList << QLatin1String( "Organizer <unittests@dev.nul>" );

    toCcList.clear();
    toCcList << QLatin1String( "opt <optional@foo.org>" )
             << QLatin1String( "non <non@foo.org>" );
    QTest::newRow("Invalid transport") << incidence << identity << /*bccMe*/true << attachment
                                       << transport  << expectedResult
                                       << expectedTransportId << expectedFrom
                                       << toList << toCcList << toBccList;
  }

  void testMailAttendees()
  {
    QFETCH( KCalCore::Incidence::Ptr, incidence );
    QFETCH( KPIMIdentities::Identity, identity );
    QFETCH( bool, bccMe );
    QFETCH( QString, attachment );
    QFETCH( QString, transport );
    QFETCH( MailClient::Result, expectedResult );
    QFETCH( int, expectedTransportId );
    QFETCH( QString, expectedFrom );
    QFETCH( QStringList, expectedToList  );
    QFETCH( QStringList, expectedCcList  );
    QFETCH( QStringList, expectedBccList );

    mPendingSignals = 1;
    mMailClient->mailAttendees( incidence, identity, bccMe, attachment, transport );
    waitForSignals();
    QCOMPARE( mLastResult, expectedResult );
    if ( expectedTransportId != -1 )
      QCOMPARE( mMailClient->mUnitTestResult.transportId, expectedTransportId );

    QCOMPARE( mMailClient->mUnitTestResult.from, expectedFrom );
    QCOMPARE( mMailClient->mUnitTestResult.to, expectedToList );
    QCOMPARE( mMailClient->mUnitTestResult.cc, expectedCcList );
    QCOMPARE( mMailClient->mUnitTestResult.bcc, expectedBccList );
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

    KCalCore::IncidenceBase::Ptr incidence( new KCalCore::Event() );
    KPIMIdentities::Identity identity;
    const QString from = QLatin1String( "from@kde.org" );
    bool bccMe;
    QString attachment;
    QString subject = QLatin1String( "subject1" );
    QString transport;
    MailClient::Result expectedResult = MailClient::ResultSuccess;
    const int expectedTransportId = 69372773; // from tests/unittestenv/kdehome/share/config/mailtransports
    QString expectedFrom = from; // from tests/unittestenv/kdehome/share/config/emailidentities
    KCalCore::Person::Ptr organizer( new KCalCore::Person( QLatin1String( "Organizer" ),
                                                           QLatin1String( "unittests@dev.nul" ) ) );
    incidence->setOrganizer( organizer );

    QStringList toList;
    toList << QLatin1String( "Organizer <unittests@dev.nul>" );
    QStringList toBccList;
    QString expectedSubject;
    //----------------------------------------------------------------------------------------------
    expectedSubject = subject;
    QTest::newRow("test1") << incidence << identity << from << bccMe << attachment << subject
                           << transport << expectedResult << expectedTransportId << expectedFrom
                           << toList << toBccList << expectedSubject;
    //----------------------------------------------------------------------------------------------
    expectedSubject = QLatin1String( "Free Busy Message" );
    incidence = KCalCore::IncidenceBase::Ptr( new KCalCore::FreeBusy() );
    incidence->setOrganizer( organizer );
    QTest::newRow("FreeBusy") << incidence << identity << from << bccMe << attachment << subject
                              << transport << expectedResult << expectedTransportId << expectedFrom
                              << toList << toBccList << expectedSubject;
  }

  void testMailOrganizer()
  {
    QFETCH( KCalCore::IncidenceBase::Ptr, incidence );
    QFETCH( KPIMIdentities::Identity, identity );
    QFETCH( QString, from );
    QFETCH( bool, bccMe );
    QFETCH( QString, attachment );
    QFETCH( QString, subject );
    QFETCH( QString, transport );
    QFETCH( MailClient::Result, expectedResult );
    QFETCH( int, expectedTransportId );
    QFETCH( QString, expectedFrom );
    QFETCH( QStringList, expectedToList  );
    QFETCH( QStringList, expectedBccList );
    QFETCH( QString, expectedSubject );

    mPendingSignals = 1;
    mMailClient->mailOrganizer( incidence, identity, from, bccMe, attachment, subject, transport );
    waitForSignals();
    QCOMPARE( mLastResult, expectedResult );
    if ( expectedTransportId != -1 )
      QCOMPARE( mMailClient->mUnitTestResult.transportId, expectedTransportId );

    QCOMPARE( mMailClient->mUnitTestResult.from, expectedFrom );
    QCOMPARE( mMailClient->mUnitTestResult.to, expectedToList );
    QCOMPARE( mMailClient->mUnitTestResult.bcc, expectedBccList );
    QCOMPARE( mMailClient->mUnitTestResult.message->subject()->asUnicodeString(), expectedSubject );
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
      QVERIFY( !QTestEventLoop::instance().timeout() );
    }
  }


public Q_SLOTS:
private:

};

QTEST_AKONADIMAIN( MailClientTest, GUI )

#include "mailclienttest.moc"
