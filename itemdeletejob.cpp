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
#include "collectionselectjob_p.h"
#include "item.h"
#include "job_p.h"
#include "protocolhelper_p.h"

#include <akonadi/private/imapparser_p.h>
#include <akonadi/private/imapset_p.h>
#include <akonadi/private/protocol_p.h>

#include <KLocale>

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
    try {
      command += ProtocolHelper::itemSetToByteArray( d->mItems, AKONADI_CMD_ITEMDELETE );
    } catch ( const std::exception &e ) {
      setError( Unknown );
      setErrorText( QString::fromUtf8( e.what() ) );
      emitResult();
      return;
    }
    command += '\n';
    d->writeData( command );
  } else {
    CollectionSelectJob *job = new CollectionSelectJob( d->mCollection, this );
    connect( job, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)) );
    addSubjob( job );
  }
}

#include "itemdeletejob.moc"
