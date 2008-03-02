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

#include "itemcopyjob.h"
#include "imapset.h"

using namespace Akonadi;

class ItemCopyJob::Private
{
  public:
    Item::List items;
    Collection target;
};

ItemCopyJob::ItemCopyJob(const Item & item, const Collection & target, QObject * parent) :
    Job( parent ),
    d( new Private )
{
  d->items << item;
  d->target = target;
}

ItemCopyJob::ItemCopyJob(const Item::List & items, const Collection & target, QObject * parent) :
    Job( parent ),
    d( new Private )
{
  d->items = items;
  d->target = target;
}

ItemCopyJob::~ ItemCopyJob()
{
  delete d;
}

void ItemCopyJob::doStart()
{
  QList<int> ids;
  foreach ( const Item item, d->items )
    ids << item.reference().id();
  ImapSet set;
  set.add( ids );
  QByteArray cmd( newTag() );
  cmd += " COPY ";
  cmd += set.toImapSequenceSet();
  cmd += ' ';
  cmd += QByteArray::number( d->target.id() );
  cmd += '\n';
  writeData( cmd );
}

#include "itemcopyjob.moc"
