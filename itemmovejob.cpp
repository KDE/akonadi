/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "itemmovejob.h"

#include "collection.h"
#include "item.h"
#include "job_p.h"
#include "protocolhelper_p.h"
#include <akonadi/private/imapparser_p.h>

using namespace Akonadi;

class Akonadi::ItemMoveJobPrivate: public JobPrivate
{
  public:
    ItemMoveJobPrivate( ItemMoveJob *parent )
      : JobPrivate( parent )
    {
    }

    Collection mTarget;
    Item::List mItems;
};

ItemMoveJob::ItemMoveJob(const Item & item, const Collection & target, QObject * parent) :
    Job( new ItemMoveJobPrivate( this ), parent )
{
  Q_D( ItemMoveJob );
  d->mTarget = target;
  d->mItems << item;
}

ItemMoveJob::ItemMoveJob( const Item::List& items, const Collection& target, QObject* parent) :
  Job( new ItemMoveJobPrivate( this ), parent )
{
  Q_D( ItemMoveJob );
  d->mTarget = target;
  d->mItems = items;
}

ItemMoveJob::~ ItemMoveJob()
{
}

void ItemMoveJob::doStart()
{
  Q_D( ItemMoveJob );
  if ( d->mItems.isEmpty() ) {
    setError( Unknown );
    setErrorText( QLatin1String( "No items specified for moving" ) );
    emitResult();
    return;
  }
  if ( !d->mTarget.isValid() && d->mTarget.remoteId().isEmpty() ) {
    setError( Unknown );
    setErrorText( QLatin1String( "No valid destination specifieed" ) );
    emitResult();
    return;
  }

  QByteArray command = d->newTag();
  try {
    command += ProtocolHelper::entitySetToByteArray( d->mItems, "MOVE" );
  } catch ( const std::exception &e ) {
    setError( Unknown );
    setErrorText( QString::fromUtf8( e.what() ) );
    emitResult();
    return;
  }
  command += ' ';
  if ( d->mItems.first().isValid() ) // with all the checks done before this indicates now whether this is a UID or RID based operation
    command += QByteArray::number( d->mTarget.id() );
  else
    command += ImapParser::quote( d->mTarget.remoteId().toUtf8() );
  command += '\n';
  d->writeData( command );
}

#include "itemmovejob.moc"
