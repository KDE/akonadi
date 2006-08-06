/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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
#include "message.h"
#include "messagejobtest.h"
#include "messagefetchjob.h"

#include "kmime_message.h"
#include "kmime_headers.h"

#include <QDebug>

using namespace PIM;

#include <qtest_kde.h>

QTEST_KDEMAIN( MessageJobTest, NoGUI );

void MessageJobTest::testMessageFetch( )
{
  // listing of an empty folder
  MessageFetchJob *job = new MessageFetchJob( "res2/foo2", this );
  QVERIFY( job->exec() );
  QVERIFY( job->messages().isEmpty() );

  // listing of a non-empty folder
  job = new MessageFetchJob( "res1/foo", this );
  QVERIFY( job->exec() );
  Message::List msgs = job->messages();
  QCOMPARE( msgs.count(), 15 );

  // check if the fetch response is parsed correctly
  Message *msg = msgs[0];
  QCOMPARE( msg->flags().count(), 3 );
  QVERIFY( msg->hasFlag( "\\Answered" ) );
  QVERIFY( msg->hasFlag( "\\Flagged" ) );
  QVERIFY( msg->hasFlag( "\\Deleted" ) );

  // TODO: complete checks for message parsing
  QCOMPARE( msg->mime()->subject()->asUnicodeString(), QString::fromUtf8( "IMPORTANT: Akonadi Test" ) );
  QCOMPARE( msg->mime()->from()->asUnicodeString(), QString::fromUtf8( "Tobias Koenig <tokoe@kde.org>" ) );
  QCOMPARE( msg->mime()->to()->asUnicodeString(), QString::fromUtf8( "Ingo Kloecker <kloecker@kde.org>" ) );

  msg = msgs[1];
  QCOMPARE( msg->flags().count(), 1 );
  QVERIFY( msg->hasFlag( "\\Answered" ) );

  msg = msgs[2];
  QVERIFY( msg->flags().isEmpty() );
}

void MessageJobTest::testIllegalMessageFetch( )
{
  // fetch non-existing folder
  MessageFetchJob *job = new MessageFetchJob( "try/to/find/me", this );
  QVERIFY( !job->exec() );

  // fetch listing of a \Noselect folder
  job = new MessageFetchJob( "res1", this );
  QVERIFY( !job->exec() );

  // fetch listing of virtual folder root
  job = new MessageFetchJob( Collection::searchFolder(), this );
  QVERIFY( !job->exec() );

  // listing of root
  job = new MessageFetchJob( Collection::root(), this );
  QVERIFY( !job->exec() );

  // fetch a non-existing message
  DataReference ref( "42", QString() );
  job = new MessageFetchJob( ref, this );
  QVERIFY( !job->exec() );

  // fetch message with empty reference
  ref = DataReference();
  job = new MessageFetchJob( ref, this );
  QVERIFY( !job->exec() );

  // fetch message with broken reference
  ref = DataReference( "some_random_textual_uid", QString() );
  job = new MessageFetchJob( ref, this );
  QVERIFY( !job->exec() );
}


#include "messagejobtest.moc"
