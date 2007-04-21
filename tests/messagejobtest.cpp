/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "collection.h"
#include "control.h"
#include "message.h"
#include "messagejobtest.h"
#include "messagefetchjob.h"
#include "collectionlistjob.h"

#include <kmime/kmime_message.h>
#include <kmime/kmime_headers.h>

#include <QtCore/QDebug>

using namespace Akonadi;

#include <qtest_kde.h>

QTEST_KDEMAIN( MessageJobTest, NoGUI )

static Collection testFolder1;
static Collection testFolder2;

void MessageJobTest::initTestCase()
{
  Control::start();

  // get the collections we run the tests on
  CollectionListJob *job = new CollectionListJob( Collection::root(), CollectionListJob::Recursive );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();
  foreach ( const Collection col, list )
    if ( col.name() == "res1" )
      testFolder1 = col;
  foreach ( const Collection col, list )
    if ( col.name() == "foo" && col.parent() == testFolder1.id() )
      testFolder2 = col;
}

void MessageJobTest::testMessageFetch( )
{
  // listing of an empty folder
  MessageFetchJob *job = new MessageFetchJob( testFolder1, this );
  QVERIFY( job->exec() );
  QVERIFY( job->messages().isEmpty() );

  // listing of a non-empty folder
  job = new MessageFetchJob( testFolder2, this );
  QVERIFY( job->exec() );
  Message::List msgs = job->messages();
  QCOMPARE( msgs.count(), 15 );

  // check if the fetch response is parsed correctly
  Message *msg = msgs[0];
  QCOMPARE( msg->reference().remoteId(), QString( "A" ) );

  QCOMPARE( msg->flags().count(), 3 );
  QVERIFY( msg->hasFlag( "\\Seen" ) );
  QVERIFY( msg->hasFlag( "\\Flagged" ) );
  QVERIFY( msg->hasFlag( "\\Draft" ) );

  // TODO: complete checks for message parsing
  QCOMPARE( msg->mime()->subject()->asUnicodeString(), QString::fromUtf8( "IMPORTANT: Akonadi Test" ) );
  QCOMPARE( msg->mime()->from()->asUnicodeString(), QString::fromUtf8( "Tobias Koenig <tokoe@kde.org>" ) );
  QCOMPARE( msg->mime()->to()->asUnicodeString(), QString::fromUtf8( "Ingo Kloecker <kloecker@kde.org>" ) );

  msg = msgs[1];
  QCOMPARE( msg->flags().count(), 1 );
  QVERIFY( msg->hasFlag( "\\Flagged" ) );

  msg = msgs[2];
  QVERIFY( msg->flags().isEmpty() );
}

void MessageJobTest::testIllegalMessageFetch( )
{
  // fetch non-existing folder
  MessageFetchJob *job = new MessageFetchJob( Collection( INT_MAX ), this );
  QVERIFY( !job->exec() );

  // listing of root
  job = new MessageFetchJob( Collection::root(), this );
  QVERIFY( !job->exec() );

  // fetch a non-existing message
  DataReference ref( 42, QString() );
  job = new MessageFetchJob( ref, this );
  QVERIFY( job->exec() );
  QVERIFY( job->messages().isEmpty() );

  // fetch message with empty reference
  ref = DataReference();
  job = new MessageFetchJob( ref, this );
  QVERIFY( !job->exec() );

  // fetch message with broken reference
  ref = DataReference( 999999, QString() );
  job = new MessageFetchJob( ref, this );
  QVERIFY( job->exec() );
  QVERIFY( job->messages().isEmpty() );
}

#include "messagejobtest.moc"
