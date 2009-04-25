/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "itemdeletejob.h"

#include "collection.h"
#include "collectionselectjob.h"
#include "item.h"
#include "job_p.h"

#include <akonadi/private/imapparser_p.h>
#include <akonadi/private/imapset_p.h>
#include <akonadi/private/protocol_p.h>

#include <KLocale>

#include <boost/bind.hpp>
#include <algorithm>

using namespace Akonadi;

class Akonadi::ItemDeleteJobPrivate : public JobPrivate
{
  public:
    ItemDeleteJobPrivate( ItemDeleteJob *parent )
      : JobPrivate( parent )
    {
    }

    void selectResult( KJob *job );

    Q_DECLARE_PUBLIC( ItemDeleteJob )

    Item::List mItems;
    Collection mCollection;
};

void ItemDeleteJobPrivate::selectResult( KJob *job )
{
  if ( job->error() )
    return; // KCompositeJob takes care of errors

  const QByteArray command = newTag() + " " AKONADI_CMD_ITEMDELETE " 1:*\n";
  writeData( command );
}

ItemDeleteJob::ItemDeleteJob( const Item & item, QObject * parent )
  : Job( new ItemDeleteJobPrivate( this ), parent )
{
  Q_D( ItemDeleteJob );

  d->mItems << item;
}

ItemDeleteJob::ItemDeleteJob(const Item::List& items, QObject* parent)
  : Job( new ItemDeleteJobPrivate( this ), parent )
{
  Q_D( ItemDeleteJob );
  d->mItems = items;
}

ItemDeleteJob::ItemDeleteJob(const Collection& collection, QObject* parent)
  : Job( new ItemDeleteJobPrivate( this ), parent )
{
  Q_D( ItemDeleteJob );
  d->mCollection = collection;
}

ItemDeleteJob::~ItemDeleteJob()
{
}

void ItemDeleteJob::doStart()
{
  Q_D( ItemDeleteJob );

  if ( !d->mItems.isEmpty() ) {
    QByteArray command = d->newTag();
    std::sort( d->mItems.begin(), d->mItems.end(), boost::bind( &Item::id, _1 ) < boost::bind( &Item::id, _2 ) );
    if ( d->mItems.first().isValid() ) {
      // all items have a uid set
      command += " " AKONADI_CMD_UID " " AKONADI_CMD_ITEMDELETE " ";
      QList<Item::Id>  uids;
      foreach ( const Item &item, d->mItems )
        uids << item.id();
      ImapSet set;
      set.add( uids );
      command += set.toImapSequenceSet();
    } else {
      // delete by remote identifier
      foreach ( const Item &item, d->mItems ) {
        if ( item.remoteId().isEmpty() ) {
          setError( Unknown );
          setErrorText( i18n( "No remote identifier specified" ) );
          emitResult();
          return;
        }
      }

      command += " " AKONADI_CMD_RID " " AKONADI_CMD_ITEMDELETE " ";
      Q_ASSERT( d->mItems.count() == 1 ); // TODO implement support for multiple items
      command += ImapParser::quote( d->mItems.first().remoteId().toUtf8() );
    }
    command += '\n';
    d->writeData( command );
  } else {
    CollectionSelectJob *job = new CollectionSelectJob( d->mCollection, this );
    connect( job, SIGNAL(result(KJob*)), SLOT(selectDone(KJob*)) );
    addSubjob( job );
  }
}

#include "itemdeletejob.moc"
