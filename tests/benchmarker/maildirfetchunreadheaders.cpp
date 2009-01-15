#include "maildirfetchunreadheaders.h"
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

typedef boost::shared_ptr<KMime::Message> MessagePtr;

using namespace Akonadi;

MailDirFetchUnreadHeaders::MailDirFetchUnreadHeaders():MailDir(){}

void MailDirFetchUnreadHeaders::runTest() {
  timer.start();
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
