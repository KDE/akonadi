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
#include <QtCore/QDebug>

using namespace Akonadi;

class ItemStoreJob::Private
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

    Private( ItemStoreJob *parent )
      : mParent( parent )
    {
    }

    ItemStoreJob *mParent;
    Item::Flags flags;
    Item::Flags addFlags;
    Item::Flags removeFlags;
    QSet<int> operations;
    QByteArray tag;
    Collection collection;
    Item item;
    QStringList parts;
    QByteArray pendingData;

    void sendNextCommand();

    QByteArray joinFlags( const Item::Flags &flags )
    {
      QByteArray rv;
      if ( flags.isEmpty() )
        return rv;
      foreach ( QByteArray f, flags )
        rv += f + ' ';
      rv.resize( rv.length() - 1 );
      return rv;
    }
};

void ItemStoreJob::Private::sendNextCommand()
{
  if ( operations.isEmpty() && parts.isEmpty() ) {
      mParent->emitResult();
      return;
  }


  tag = mParent->newTag();
  QByteArray command = tag;
  command += " UID STORE " + QByteArray::number( item.reference().id() ) + ' ';
  if ( !operations.isEmpty() ) {
    int op = *(operations.begin());
    operations.remove( op );
    switch ( op ) {
      case SetFlags:
        command += "FLAGS (" + joinFlags( flags ) + ')';
        break;
      case AddFlags:
        command += "+FLAGS (" + joinFlags( addFlags ) + ')';
        break;
      case RemoveFlags:
        command += "-FLAGS (" + joinFlags( removeFlags ) + ')';
        break;
      case Move:
        command += "COLLECTION " + QByteArray::number( collection.id() );
        break;
      case RemoteId:
        if ( item.reference().remoteId().isNull() ) {
          sendNextCommand();
          return;
        }
        command += "REMOTEID \"" + item.reference().remoteId().toLatin1() + '\"';
        break;
      case Dirty:
        command += "DIRTY";
        break;
    }
  } else {
    QString label = parts.takeFirst();
    pendingData = item.part( label );
    command += label.toUtf8();
    command += " {" + QByteArray::number( pendingData.size() ) + '}';
  }
  command += '\n';
  mParent->writeData( command );
  mParent->newTag(); // hack to circumvent automatic response handling
}


ItemStoreJob::ItemStoreJob(const Item & item, QObject * parent) :
    Job( parent ),
    d( new Private( this ) )
{
  d->item = item;
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

#include "itemstorejob.moc"
