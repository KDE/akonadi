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

#include "itemmodifyjob.h"
#include "itemmodifyjob_p.h"

#include "collection.h"
#include "entity_p.h"
#include "imapparser_p.h"
#include "itemserializer.h"
#include "job_p.h"
#include "item_p.h"

#include <kdebug.h>

using namespace Akonadi;

ItemModifyJobPrivate::ItemModifyJobPrivate( ItemModifyJob *parent, const Item &item )
  : JobPrivate( parent ),
    mItem( item ),
    mRevCheck( true )
{
}

void ItemModifyJobPrivate::setClean()
{
  mOperations.insert( Dirty );
}

QByteArray ItemModifyJobPrivate::nextPartHeader()
{
  QByteArray command;
  if ( !mParts.isEmpty() ) {
    QString label = mParts.takeFirst();
    mPendingData.clear();
    ItemSerializer::serialize( mItem, label, mPendingData );
    command += ' ' + label.toUtf8();
    command += ".SILENT {" + QByteArray::number( mPendingData.size() ) + '}';
    if ( mPendingData.size() > 0 )
      command += '\n';
    else
      command += nextPartHeader();
  } else {
    command += ")\n";
  }
  return command;
}


ItemModifyJob::ItemModifyJob( const Item &item, QObject * parent )
  : Job( new ItemModifyJobPrivate( this, item ), parent )
{
  Q_D( ItemModifyJob );

  d->mOperations.insert( ItemModifyJobPrivate::RemoteId );
}

ItemModifyJob::~ItemModifyJob()
{
}

void ItemModifyJob::doStart()
{
  Q_D( ItemModifyJob );

  QList<QByteArray> changes;
  foreach ( int op, d->mOperations ) {
    switch ( op ) {
      case ItemModifyJobPrivate::RemoteId:
        if ( !d->mItem.remoteId().isNull() ) {
          changes << "REMOTEID.SILENT";
          changes << ImapParser::quote( d->mItem.remoteId().toLatin1() );
        }
        break;
      case ItemModifyJobPrivate::Dirty:
        changes << "DIRTY.SILENT";
        changes << "false";
        break;
    }
  }

  if ( d->mItem.d_func()->mFlagsOverwritten ) {
    changes << "FLAGS.SILENT";
    changes << '(' + ImapParser::join( d->mItem.flags(), " " ) + ')';
  } else {
    if ( !d->mItem.d_func()->mAddedFlags.isEmpty() ) {
      changes << "+FLAGS.SILENT";
      changes << '(' + ImapParser::join( d->mItem.d_func()->mAddedFlags, " " ) + ')';
    }
    if ( !d->mItem.d_func()->mDeletedFlags.isEmpty() ) {
      changes << "-FLAGS.SILENT";
      changes << '(' + ImapParser::join( d->mItem.d_func()->mDeletedFlags, " " ) + ')';
    }
  }

  if ( !d->mItem.d_func()->mDeletedAttributes.isEmpty() ) {
    changes << "-PARTS.SILENT";
    changes << '(' + ImapParser::join( d->mItem.d_func()->mDeletedAttributes, " " ) + ')';
  }

  // nothing to do
  if ( changes.isEmpty() && d->mParts.isEmpty() ) {
    emitResult();
    return;
  }

  d->mTag = d->newTag();
  QByteArray command = d->mTag;
  command += " UID STORE " + QByteArray::number( d->mItem.id() ) + ' ';
  if ( !d->mRevCheck ) {
    command += "NOREV ";
  } else {
    command += "REV " + QByteArray::number( d->mItem.revision() );
  }

  command += " (" + ImapParser::join( changes, " " );
  command += d->nextPartHeader();
  d->writeData( command );
  d->newTag(); // hack to circumvent automatic response handling
}

void ItemModifyJob::doHandleResponse(const QByteArray &_tag, const QByteArray & data)
{
  Q_D( ItemModifyJob );

  if ( _tag == "+" ) { // ready for literal data
    d->writeData( d->mPendingData );
    d->writeData( d->nextPartHeader() );
    return;
  }
  if ( _tag == d->mTag ) {
    if ( data.startsWith( "OK" ) ) {
      // increase item revision of own copy of item
      d->mItem.setRevision( d->mItem.revision() + 1 );
      d->mItem.d_ptr->resetChangeLog();
    } else {
      setError( Unknown );
      setErrorText( QString::fromUtf8( data ) );
    }
    emitResult();
    return;
  }
  kDebug( 5250 ) << "Unhandled response: " << _tag << data;
}

void ItemModifyJob::storePayload()
{
  Q_D( ItemModifyJob );

  Q_ASSERT( !d->mItem.mimeType().isEmpty() );
  d->mParts = d->mItem.availableParts();
}

void ItemModifyJob::disableRevisionCheck()
{
  Q_D( ItemModifyJob );

  d->mRevCheck = false;
}

Item ItemModifyJob::item() const
{
  Q_D( const ItemModifyJob );

  return d->mItem;
}

#include "itemmodifyjob.moc"
