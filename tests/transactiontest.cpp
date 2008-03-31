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
#include <QtGui/QApplication>
#include <QtTest/QSignalSpy>
#include <qtest_kde.h>

using namespace Akonadi;

QTEST_KDEMAIN( TransactionTest, NoGUI )

void TransactionTest::initTestCase()
{
  Control::start();
}

void TransactionTest::testTransaction()
{
  Collection basisCollection;

  CollectionFetchJob *listJob = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
  QVERIFY( listJob->exec() );
  Collection::List list = listJob->collections();
  foreach ( const Collection col, list )
    if ( col.name() == "res3" )
      basisCollection = col;

  Collection testCollection;
  testCollection.setParent( basisCollection );
  testCollection.setName( "transactionTest" );
  testCollection.setRemoteId( "transactionTestRemoteId" );
  CollectionCreateJob *job = new CollectionCreateJob( testCollection, Session::defaultSession() );

  QVERIFY( job->exec() );

  testCollection = job->collection();

  TransactionBeginJob *beginTransaction1 = new TransactionBeginJob( Session::defaultSession() );
  QVERIFY( beginTransaction1->exec() );

  TransactionBeginJob *beginTransaction2 = new TransactionBeginJob( Session::defaultSession() );
  QVERIFY( beginTransaction2->exec() );

  TransactionCommitJob *commitTransaction2 = new TransactionCommitJob( Session::defaultSession() );
  QVERIFY( commitTransaction2->exec() );

  TransactionCommitJob *commitTransaction1 = new TransactionCommitJob( Session::defaultSession() );
  QVERIFY( commitTransaction1->exec() );

  TransactionCommitJob *commitTransactionX = new TransactionCommitJob( Session::defaultSession() );
  QVERIFY( commitTransactionX->exec() == false );

  TransactionBeginJob *beginTransaction3 = new TransactionBeginJob( Session::defaultSession() );
  QVERIFY( beginTransaction3->exec() );

  Item item;
  item.setMimeType( "application/octet-stream" );
  item.setPayload<QByteArray>( "body data" );
  ItemCreateJob *appendJob = new ItemCreateJob( item, testCollection, Session::defaultSession() );
  QVERIFY( appendJob->exec() );

  TransactionRollbackJob *rollbackTransaction3 = new TransactionRollbackJob( Session::defaultSession() );
  QVERIFY( rollbackTransaction3->exec() );

  ItemFetchJob *fetchJob = new ItemFetchJob( testCollection, Session::defaultSession() );
  QVERIFY( fetchJob->exec() );

  QVERIFY( fetchJob->items().isEmpty() );

  CollectionDeleteJob *deleteJob = new CollectionDeleteJob( testCollection, Session::defaultSession() );
  QVERIFY( deleteJob->exec() );
}

#include "transactiontest.moc"
