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

#include "maildirfetchallheaders.h"
#include "maildir.h"

#include <QDebug>

#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>

#include <kmime/kmime_message.h>
#include "kmime/messageparts.h"

#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<KMime::Message> MessagePtr;

using namespace Akonadi;

MailDirFetchAllHeaders::MailDirFetchAllHeaders():MailDir(){}

void MailDirFetchAllHeaders::runTest() {
  timer.start();
  qDebug() << "  Listing all headers of every folder.";
  CollectionFetchJob *clj = new CollectionFetchJob( Collection::root() , CollectionFetchJob::Recursive );
  clj->fetchScope().setResource( currentInstance.identifier() );
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
