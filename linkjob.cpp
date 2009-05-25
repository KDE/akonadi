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

#include "linkjob.h"

#include "collection.h"
#include "job_p.h"
#include <akonadi/private/imapset_p.h>

using namespace Akonadi;

class Akonadi::LinkJobPrivate : public JobPrivate
{
  public:
    LinkJobPrivate( LinkJob *parent )
      : JobPrivate( parent )
    {
    }

    Collection collection;
    ImapSet set;
};

LinkJob::LinkJob( const Collection &collection, const Item::List &items, QObject *parent ) :
    Job( new LinkJobPrivate( this ), parent )
{
  Q_D( LinkJob );
  d->collection = collection;
  QList<Item::Id> ids;
  foreach ( const Item &item, items )
    ids << item.id();
  d->set.add( ids );
}

LinkJob::~LinkJob()
{
}

void LinkJob::doStart()
{
  Q_D( LinkJob );

  QByteArray command = d->newTag();
  command += " LINK ";
  command += QByteArray::number( d->collection.id() );
  command += ' ';
  command += d->set.toImapSequenceSet();
  command += '\n';
  d->writeData( command );
}

#include "linkjob.moc"
