/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>

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

#include "transactiontest.h"

#include <akonadi/itemcreatejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/collectioncreatejob.h>
#include <akonadi/collectiondeletejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/control.h>
#include <akonadi/session.h>
#include <akonadi/transactionjobs.h>

#include <QtCore/QVariant>
#include <QtTest/QSignalSpy>
#include <qtest_akonadi.h>

using namespace Akonadi;

QTEST_AKONADIMAIN( TransactionTest, NoGUI )

void TransactionTest::initTestCase()
{
  AkonadiTest::checkTestIsIsolated();
  Control::start();
}

void TransactionTest::testTransaction()
{
  Collection basisCollection;

  CollectionFetchJob *listJob = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
  AKVERIFYEXEC( listJob );
  Collection::List list = listJob->collections();
  foreach ( const Collection &col, list )
    if ( col.name() == "res3" )
      basisCollection = col;

  Collection testCollection;
  testCollection.setParentCollection( basisCollection );
  testCollection.setName( "transactionTest" );
  testCollection.setRemoteId( "transactionTestRemoteId" );
  CollectionCreateJob *job = new CollectionCreateJob( testCollection, Session::defaultSession() );

  AKVERIFYEXEC( job );

  testCollection = job->collection();

  TransactionBeginJob *beginTransaction1 = new TransactionBeginJob( Session::defaultSession() );
  AKVERIFYEXEC( beginTransaction1 );

  TransactionBeginJob *beginTransaction2 = new TransactionBeginJob( Session::defaultSession() );
  AKVERIFYEXEC( beginTransaction2 );

  TransactionCommitJob *commitTransaction2 = new TransactionCommitJob( Session::defaultSession() );
  AKVERIFYEXEC( commitTransaction2 );

  TransactionCommitJob *commitTransaction1 = new TransactionCommitJob( Session::defaultSession() );
  AKVERIFYEXEC( commitTransaction1 );

  TransactionCommitJob *commitTransactionX = new TransactionCommitJob( Session::defaultSession() );
  QVERIFY( commitTransactionX->exec() == false );

  TransactionBeginJob *beginTransaction3 = new TransactionBeginJob( Session::defaultSession() );
  AKVERIFYEXEC( beginTransaction3 );

  Item item;
  item.setMimeType( "application/octet-stream" );
  item.setPayload<QByteArray>( "body data" );
  ItemCreateJob *appendJob = new ItemCreateJob( item, testCollection, Session::defaultSession() );
  AKVERIFYEXEC( appendJob );

  TransactionRollbackJob *rollbackTransaction3 = new TransactionRollbackJob( Session::defaultSession() );
  AKVERIFYEXEC( rollbackTransaction3 );

  ItemFetchJob *fetchJob = new ItemFetchJob( testCollection, Session::defaultSession() );
  AKVERIFYEXEC( fetchJob );

  QVERIFY( fetchJob->items().isEmpty() );

  CollectionDeleteJob *deleteJob = new CollectionDeleteJob( testCollection, Session::defaultSession() );
  AKVERIFYEXEC( deleteJob );
}

