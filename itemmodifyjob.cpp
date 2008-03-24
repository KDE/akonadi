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

#include "collection.h"
#include "entity_p.h"
#include "imapparser_p.h"
#include "itemserializer.h"
#include "job_p.h"

#include <kdebug.h>

#include <QtCore/QStringList>

using namespace Akonadi;

class Akonadi::ItemModifyJobPrivate : public JobPrivate
{
  public:
    enum Operation {
      SetFlags,
      AddFlags,
      RemoveFlags,
      Move,
      RemoteId,
      Dirty
    };

    ItemModifyJobPrivate( ItemModifyJob *parent, const Item &item )
      : JobPrivate( parent ),
        mItem( item ),
        mRevCheck( true )
    {
    }

    Q_DECLARE_PUBLIC( ItemModifyJob )

    Item::Flags mFlags;
    Item::Flags mAddFlags;
    Item::Flags mRemoveFlags;
    QSet<int> mOperations;
    QByteArray mTag;
    Collection mCollection;
    Item mItem;
    bool mRevCheck;
    QStringList mParts;
    QByteArray mPendingData;

    QByteArray nextPartHeader()
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
};

ItemModifyJob::ItemModifyJob( const Item &item, QObject * parent )
  : Job( new ItemModifyJobPrivate( this, item ), parent )
{
  Q_D( ItemModifyJob );

  d->mOperations.insert( ItemModifyJobPrivate::RemoteId );
}

ItemModifyJob::~ItemModifyJob()
{
}

void ItemModifyJob::setFlags(const Item::Flags & flags)
{
  Q_D( ItemModifyJob );

  d->mFlags = flags;
  d->mOperations.insert( ItemModifyJobPrivate::SetFlags );
}

void ItemModifyJob::addFlag(const Item::Flag & flag)
{
  Q_D( ItemModifyJob );

  d->mAddFlags.insert( flag );
  d->mOperations.insert( ItemModifyJobPrivate::AddFlags );
}

void ItemModifyJob::removeFlag(const Item::Flag & flag)
{
  Q_D( ItemModifyJob );

  d->mRemoveFlags.insert( flag );
  d->mOperations.insert( ItemModifyJobPrivate::RemoveFlags );
}

void ItemModifyJob::setCollection(const Collection &collection)
{
  Q_D( ItemModifyJob );

  d->mCollection = collection;
  d->mOperations.insert( ItemModifyJobPrivate::Move );
}

void ItemModifyJob::setClean()
{
  Q_D( ItemModifyJob );

  d->mOperations.insert( ItemModifyJobPrivate::Dirty );
}

void ItemModifyJob::doStart()
{
  Q_D( ItemModifyJob );

  // nothing to do
  if ( d->mOperations.isEmpty() && d->mParts.isEmpty() ) {
    emitResult();
    return;
  }

  d->mTag = newTag();
  QByteArray command = d->mTag;
  command += " UID STORE " + QByteArray::number( d->mItem.id() ) + ' ';
  if ( !d->mRevCheck || d->mAddFlags.contains( "\\Deleted" ) ) {
    command += "NOREV ";
  } else {
    command += "REV " + QByteArray::number( d->mItem.revision() );
  }
  QList<QByteArray> changes;
  foreach ( int op, d->mOperations ) {
    switch ( op ) {
      case ItemModifyJobPrivate::SetFlags:
        changes << "FLAGS.SILENT";
        changes << "(" + ImapParser::join( d->mFlags, " " ) + ')';
        break;
      case ItemModifyJobPrivate::AddFlags:
        changes << "+FLAGS.SILENT";
        changes << "(" + ImapParser::join( d->mAddFlags, " " ) + ')';
        break;
      case ItemModifyJobPrivate::RemoveFlags:
        changes << "-FLAGS.SILENT";
        changes << "(" + ImapParser::join( d->mRemoveFlags, " " ) + ')';
        break;
      case ItemModifyJobPrivate::Move:
        changes << "COLLECTION.SILENT";
        changes << QByteArray::number( d->mCollection.id() );
        break;
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

  if ( !d->mItem.d_ptr->mDeletedAttributes.isEmpty() ) {
    changes << "-PARTS.SILENT";
    changes << "(" + ImapParser::join( d->mItem.d_ptr->mDeletedAttributes, " " ) + ')';
  }

  command += " (" + ImapParser::join( changes, " " );
  command += d->nextPartHeader();
  writeData( command );
  newTag(); // hack to circumvent automatic response handling
}

void ItemModifyJob::doHandleResponse(const QByteArray &_tag, const QByteArray & data)
{
  Q_D( ItemModifyJob );

  if ( _tag == "+" ) { // ready for literal data
    writeData( d->mPendingData );
    writeData( d->nextPartHeader() );
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
