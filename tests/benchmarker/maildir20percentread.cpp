#include "maildir20percentread.h"
#include "maildir.h"

#include <QDebug>
#include <QTest>

#include <akonadi/collectiondeletejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>

using namespace Akonadi;

MailDir20PercentAsRead::MailDir20PercentAsRead():MailDir(){}

void MailDir20PercentAsRead::runTest() {
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


