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

#include "itemstorejob.h"
#include "imapparser.h"
#include <QtCore/QDebug>

using namespace Akonadi;

class ItemStoreJob::Private
{
  public:
    enum Operation {
      SetFlags,
      AddFlags,
      RemoveFlags,
      RemoveParts,
      Move,
      RemoteId,
      Dirty
    };

    Private( ItemStoreJob *parent, Item & it )
    : mParent( parent ), item( it ), itemRef( it ), revCheck( true )
    {
    }

    Private( ItemStoreJob *parent, const Item &it )
    : mParent( parent ), item( it ), itemRef( item ), revCheck( true )
    {
    }

    ItemStoreJob *mParent;
    Item::Flags flags;
    Item::Flags addFlags;
    Item::Flags removeFlags;
    QList<QByteArray> removeParts;
    QSet<int> operations;
    QByteArray tag;
    Collection collection;
    Item item;
    Item & itemRef; // used for increasing revision number of given item
    bool revCheck;
    QStringList parts;
    QByteArray pendingData;

    void sendNextCommand();
};

void ItemStoreJob::Private::sendNextCommand()
{
  // no further commands to send
  if ( operations.isEmpty() && parts.isEmpty() ) {
    mParent->emitResult();
    return;
  }

  tag = mParent->newTag();
  QByteArray command = tag;
  command += " UID STORE " + QByteArray::number( item.reference().id() ) + ' ';
  if ( !revCheck || addFlags.contains( "\\Deleted" ) ) {
    command += "NOREV ";
  } else {
    command += "REV " + QByteArray::number( itemRef.rev() ) + ' ';
  }
  if ( !operations.isEmpty() ) {
    int op = *(operations.begin());
    operations.remove( op );
    switch ( op ) {
      case SetFlags:
        command += "FLAGS.SILENT (" + ImapParser::join( flags, " " ) + ')';
        break;
      case AddFlags:
        command += "+FLAGS.SILENT (" + ImapParser::join( addFlags, " " ) + ')';
        break;
      case RemoveFlags:
        command += "-FLAGS.SILENT (" + ImapParser::join( removeFlags, " " ) + ')';
        break;
      case RemoveParts:
        command += "-PARTS.SILENT (" + ImapParser::join( removeParts, " " ) + ')';
        break;
      case Move:
        command += "COLLECTION.SILENT " + QByteArray::number( collection.id() );
        break;
      case RemoteId:
        if ( item.reference().remoteId().isNull() ) {
          sendNextCommand();
          return;
        }
        command += "REMOTEID.SILENT \"" + item.reference().remoteId().toLatin1() + '\"';
        break;
      case Dirty:
        command += "DIRTY.SILENT";
        break;
    }
  } else {
    QString label = parts.takeFirst();
    pendingData = item.part( label );
    command += label.toUtf8();
    command += ".SILENT {" + QByteArray::number( pendingData.size() ) + '}';
  }
  command += '\n';
  mParent->writeData( command );
  mParent->newTag(); // hack to circumvent automatic response handling
}


ItemStoreJob::ItemStoreJob(Item & item, QObject * parent) :
    Job( parent ),
    d( new Private( this, item ) )
{
  d->operations.insert( Private::RemoteId );
}

ItemStoreJob::ItemStoreJob(const Item &item, QObject * parent) :
    Job( parent ),
    d( new Private( this, item ) )
{
  d->operations.insert( Private::RemoteId );
}

ItemStoreJob::~ ItemStoreJob()
{
  delete d;
}

void ItemStoreJob::setFlags(const Item::Flags & flags)
{
  d->flags = flags;
  d->operations.insert( Private::SetFlags );
}

void ItemStoreJob::addFlag(const Item::Flag & flag)
{
  d->addFlags.insert( flag );
  d->operations.insert( Private::AddFlags );
}

void ItemStoreJob::removeFlag(const Item::Flag & flag)
{
  d->removeFlags.insert( flag );
  d->operations.insert( Private::RemoveFlags );
}

void ItemStoreJob::removePart(const QByteArray & part)
{
  d->removeParts.append( part );
  d->operations.insert( Private::RemoveParts );
}

void ItemStoreJob::setCollection(const Collection &collection)
{
  d->collection = collection;
  d->operations.insert( Private::Move );
}

void ItemStoreJob::setClean()
{
  d->operations.insert( Private::Dirty );
}

void ItemStoreJob::doStart()
{
  d->sendNextCommand();
}

void ItemStoreJob::doHandleResponse(const QByteArray &_tag, const QByteArray & data)
{
  if ( _tag == "+" ) { // ready for literal data
    writeData( d->pendingData );
    // ### readLine() would deadlock in the server otherwise, should probably be fixed there
    if ( !d->pendingData.endsWith( '\n' ) )
      writeData( "\n" );
    return;
  }
  if ( _tag == d->tag ) {
    if ( data.startsWith( "OK" ) ) {
      // increase item revision of item, given by calling function
      if( d->revCheck )
        d->itemRef.incRev();
      else
        // increase item revision of own copy of item
        d->item.incRev();

      d->sendNextCommand();
    } else {
      setError( Unknown );
      setErrorText( QString::fromUtf8( data ) );
      emitResult();
    }
    return;
  }
  qDebug() << "unhandled response in item store job: " << _tag << data;
}

void ItemStoreJob::storePayload()
{
  Q_ASSERT( !d->item.mimeType().isEmpty() );
  d->parts = d->item.availableParts();
}

void ItemStoreJob::noRevCheck()
{
  d->revCheck = false;
}

#include "itemstorejob.moc"
