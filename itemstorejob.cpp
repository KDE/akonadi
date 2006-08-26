/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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
#include <QDebug>

using namespace PIM;

class PIM::ItemStoreJobPrivate
{
  public:
    enum Operation {
      Data,
      SetFlags,
      AddFlags,
      RemoveFlags
    };

    Item::Flags flags;
    Item::Flags addFlags;
    Item::Flags removeFlags;
    QByteArray data;
    DataReference ref;
    QSet<int> operations;
    QByteArray tag;

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

PIM::ItemStoreJob::ItemStoreJob(Item * item, QObject * parent) :
    Job ( parent ),
    d( new ItemStoreJobPrivate )
{
  Q_ASSERT( item );
  d->flags = item->flags();
  d->ref = item->reference();
  d->data = item->data();
  d->operations.insert( ItemStoreJobPrivate::Data );
  d->operations.insert( ItemStoreJobPrivate::SetFlags );
}

PIM::ItemStoreJob::ItemStoreJob(const DataReference &ref, QObject * parent) :
    Job( parent ),
    d( new ItemStoreJobPrivate )
{
  d->ref = ref;
}

PIM::ItemStoreJob::~ ItemStoreJob()
{
  delete d;
}

void PIM::ItemStoreJob::setData(const QByteArray & data)
{
  d->data = data;
  d->operations.insert( ItemStoreJobPrivate::Data );
}

void PIM::ItemStoreJob::setFlags(const Item::Flags & flags)
{
  d->flags = flags;
  d->operations.insert( ItemStoreJobPrivate::SetFlags );
}

void PIM::ItemStoreJob::addFlag(const Item::Flag & flag)
{
  d->addFlags.insert( flag );
  d->operations.insert( ItemStoreJobPrivate::AddFlags );
}

void PIM::ItemStoreJob::removeFlag(const Item::Flag & flag)
{
  d->removeFlags.insert( flag );
  d->operations.insert( ItemStoreJobPrivate::RemoveFlags );
}

void PIM::ItemStoreJob::doStart()
{
  sendNextCommand();
}

void PIM::ItemStoreJob::doHandleResponse(const QByteArray &_tag, const QByteArray & data)
{
  if ( _tag == "+" ) { // ready for literal data
    writeData( d->data );
    return;
  }
  if ( _tag == d->tag ) {
    if ( data.startsWith( "OK" ) ) {
      sendNextCommand();
    } else {
      setError( Unknown, data );
      emit done( this );
    }
    return;
  }
  qDebug() << "unhandled response in item store job: " << _tag << data;
}

void PIM::ItemStoreJob::sendNextCommand()
{
  if ( d->operations.isEmpty() ) {
    emit done( this );
    return;
  }

  d->tag = newTag();
  QByteArray command = d->tag;
  command += " UID STORE " + d->ref.persistanceID().toLatin1() + ' ';
  int op = *(d->operations.begin());
  d->operations.remove( op );
  switch ( op ) {
    case ItemStoreJobPrivate::Data:
      command += "DATA {" + QByteArray::number( d->data.size() ) + '}';
      break;
    case ItemStoreJobPrivate::SetFlags:
      command += "FLAGS (" + d->joinFlags( d->flags ) + ')';
      break;
    case ItemStoreJobPrivate::AddFlags:
      command += "+FLAGS (" + d->joinFlags( d->addFlags ) + ')';
      break;
    case ItemStoreJobPrivate::RemoveFlags:
      command += "-FLAGS (" + d->joinFlags( d->removeFlags ) + ')';
      break;
  }
  writeData( command );
  newTag(); // hack to circumvent automatic response handling
}

#include "itemstorejob.moc"
