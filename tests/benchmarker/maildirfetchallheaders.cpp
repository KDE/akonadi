#include "maildirfetchallheaders.h"
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

#include <boost/shared_ptr.hpp>

#define WAIT_TIME 100

typedef boost::shared_ptr<KMime::Message> MessagePtr;

using namespace Akonadi;

MailDirFetchAllHeaders::MailDirFetchAllHeaders():MailDir(){}

void MailDirFetchAllHeaders::runTest() {
  timer.start();
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
