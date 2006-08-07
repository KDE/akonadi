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

using namespace PIM;

class PIM::ItemStoreJobPrivate
{
  public:
    Item::Flags flags;
    DataReference ref;
};

PIM::ItemStoreJob::ItemStoreJob(Item * item, QObject * parent) :
    Job ( parent ),
    d( new ItemStoreJobPrivate )
{
  Q_ASSERT( item );
  d->flags = item->flags();
  d->ref = item->reference();
}

PIM::ItemStoreJob::ItemStoreJob(const DataReference &ref, const Item::Flags & flags, QObject * parent) :
    Job( parent ),
    d( new ItemStoreJobPrivate )
{
  d->flags = flags;
  d->ref = ref;
}

PIM::ItemStoreJob::~ ItemStoreJob()
{
  delete d;
}

void PIM::ItemStoreJob::doStart()
{
  QByteArray command = newTag();
  command += " UID STORE " + d->ref.persistanceID().toLatin1();
  command += " FLAGS (";
  foreach ( QByteArray f, d->flags )
    command += f + ' ';
  command += ')';
  writeData( command );
}

void PIM::ItemStoreJob::doHandleResponse(const QByteArray & tag, const QByteArray & data)
{
  // we don't care for now
  Q_UNUSED( tag );
  Q_UNUSED( data );
}

#include "itemstorejob.moc"
