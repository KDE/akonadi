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

#include "maildir20percentread.h"
#include "maildir.h"

#include <QDebug>

#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemmodifyjob.h>

using namespace Akonadi;

MailDir20PercentAsRead::MailDir20PercentAsRead():MailDir(){}

void MailDir20PercentAsRead::runTest() {
  timer.start();
  qDebug() << "  Marking 20% of messages as read.";
  CollectionFetchJob *clj2 = new CollectionFetchJob( Collection::root() , CollectionFetchJob::Recursive );
  clj2->fetchScope().setResource( currentInstance.identifier() );
  clj2->exec();
  Collection::List list2 = clj2->collections();
  foreach ( const Collection &collection, list2 ) {
    ItemFetchJob *ifj = new ItemFetchJob( collection, this );
    ifj->exec();
    Item::List itemlist = ifj->items();
    for ( int i = ifj->items().count() - 1; i >= 0; i -= 5) {
      Item item = itemlist[i];
      item.setFlag( "\\SEEN" );
      ItemModifyJob *isj = new ItemModifyJob( item, this );
      isj->exec();
    }
  }
  outputStats( "mark20percentread" );
}
