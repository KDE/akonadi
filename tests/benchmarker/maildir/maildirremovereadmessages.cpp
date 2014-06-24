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

#include "maildirremovereadmessages.h"
#include "maildir.h"
#include <QDebug>

#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>

using namespace Akonadi;

MailDirRemoveReadMessages::MailDirRemoveReadMessages():MailDir(){}

void MailDirRemoveReadMessages::runTest() {
  timer.start();
  qDebug() << "  Removing read messages from every folder.";
  CollectionFetchJob *clj4 = new CollectionFetchJob( Collection::root() , CollectionFetchJob::Recursive );
  clj4->fetchScope().setResource( currentInstance.identifier() );
  clj4->exec();
  Collection::List list4 = clj4->collections();
  foreach ( const Collection &collection, list4 ) {
    ItemFetchJob *ifj = new ItemFetchJob( collection, this );
    ifj->exec();
    foreach ( const Item &item, ifj->items() ) {
      // delete read messages
      if ( item.hasFlag( "\\SEEN" ) ) {
        ItemDeleteJob *idj = new ItemDeleteJob( item, this);
        idj->exec();
      }
    }
  }
  outputStats( "removereaditems" );

}
