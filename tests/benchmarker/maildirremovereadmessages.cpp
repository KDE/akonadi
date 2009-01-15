#include "maildirremovereadmessages.h"
#include "maildir.h"
#include <QDebug>
#include <QTest>

#include <akonadi/collectiondeletejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>

#include <kmime/kmime_message.h>
#include "kmime/messageparts.h"

using namespace Akonadi;

MailDirRemoveReadMessages::MailDirRemoveReadMessages():MailDir(){}

void MailDirRemoveReadMessages::runTest() {
  timer.start();
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


