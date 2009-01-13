/*
    Copyright (c) 2009 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
    based on kdepimlibs/akonadi/tests/benchmarker.cpp wrote by Robert Zwerus <arzie@dds.nl>

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

#include "testmaildir.h"

#include <akonadi/collectiondeletejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>

#include <kmime/kmime_message.h>
#include "kmime/messageparts.h"

#include <QDebug>
#include <QTest>

#include <boost/shared_ptr.hpp>

#define WAIT_TIME 100

typedef boost::shared_ptr<KMime::Message> MessagePtr;

using namespace Akonadi;

TestMailDir::TestMailDir(const QString &dir) : Test()
{
  createAgent("akonadi_maildir_resource");
  configureDBusIface("Maildir",dir);

}

void TestMailDir::runBenchMarker()
{
  importMailDir();
  fetchAllHeaders();
  mark20PercentAsRead();
  fetchUnreadHeaders();
  removeAllReadMessages();
  removeCollections();
  removeResource();

}

void TestMailDir::importMailDir()
{
  done = false;
  timer.start();
  qDebug() << "  Synchronising resource.";
  currentInstance.synchronize();
  while(!done)
    QTest::qWait( WAIT_TIME );
  outputStats( "import" );
}

void TestMailDir::fetchAllHeaders()
{
  timer.restart();
  qDebug() << "  Listing all headers of every folder.";
  CollectionFetchJob *clj = new CollectionFetchJob( Collection::root() , CollectionFetchJob::Recursive );
  clj->setResource( currentInstance.identifier() );
  clj->exec();
  Collection::List list = clj->collections();
  foreach ( const Collection &collection, list ) {
    ItemFetchJob *ifj = new ItemFetchJob( collection, this );
    ifj->fetchScope().fetchPayloadPart( MessagePart::Envelope );
    ifj->exec();
    QString a;
    foreach ( const Item &item, ifj->items() ) {
      a = item.payload<MessagePtr>()->subject()->asUnicodeString();
    }
  }
  outputStats( "fullheaderlist" );
}

void TestMailDir::mark20PercentAsRead()
{
  timer.restart();
  qDebug() << "  Marking 20% of messages as read.";
  CollectionFetchJob *clj2 = new CollectionFetchJob( Collection::root() , CollectionFetchJob::Recursive );
  clj2->setResource( currentInstance.identifier() );
  clj2->exec();
  Collection::List list2 = clj2->collections();
  foreach ( const Collection &collection, list2 ) {
    ItemFetchJob *ifj = new ItemFetchJob( collection, this );
    ifj->exec();
    Item::List itemlist = ifj->items();
    for ( int i = ifj->items().count() - 1; i >= 0; i -= 5) {
      Item item = itemlist[i];
      item.setFlag( "\\Seen" );
      ItemModifyJob *isj = new ItemModifyJob( item, this );
      isj->exec();
    }
  }
  outputStats( "mark20percentread" );
}

void TestMailDir::fetchUnreadHeaders()
{
  timer.restart();
  qDebug() << "  Listing headers of unread messages of every folder.";
  CollectionFetchJob *clj3 = new CollectionFetchJob( Collection::root() , CollectionFetchJob::Recursive );
  clj3->setResource( currentInstance.identifier() );
  clj3->exec();
  Collection::List list3 = clj3->collections();
  foreach ( const Collection &collection, list3 ) {
    ItemFetchJob *ifj = new ItemFetchJob( collection, this );
    ifj->fetchScope().fetchPayloadPart( MessagePart::Envelope );
    ifj->exec();
    QString a;
    foreach ( const Item &item, ifj->items() ) {
      // filter read messages
      if( !item.hasFlag( "\\Seen" ) ) {
        a = item.payload<MessagePtr>()->subject()->asUnicodeString();
      }
    }
  }
  outputStats( "unreadheaderlist" );
}

void TestMailDir::removeAllReadMessages()
{
  timer.restart();
  qDebug() << "  Removing read messages from every folder.";
  CollectionFetchJob *clj4 = new CollectionFetchJob( Collection::root() , CollectionFetchJob::Recursive );
  clj4->setResource( currentInstance.identifier() );
  clj4->exec();
  Collection::List list4 = clj4->collections();
  foreach ( const Collection &collection, list4 ) {
    ItemFetchJob *ifj = new ItemFetchJob( collection, this );
    ifj->exec();
    foreach ( const Item &item, ifj->items() ) {
      // delete read messages
      if( item.hasFlag( "\\Seen" ) ) {
        ItemDeleteJob *idj = new ItemDeleteJob( item, this);
        idj->exec();
      }
    }
  }
  outputStats( "removereaditems" );
}
