/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "itemsearchjob.h"

#include "imapparser_p.h"
#include "job_p.h"

using namespace Akonadi;

class Akonadi::ItemSearchJobPrivate : public JobPrivate
{
  public:
    ItemSearchJobPrivate( ItemSearchJob *parent, const QString &query )
      : JobPrivate( parent ), mQuery( query )
    {
    }

    QString mQuery;
    Item::List mItems;
};

ItemSearchJob::ItemSearchJob( const QString & query, QObject * parent )
  : Job( new ItemSearchJobPrivate( this, query ), parent )
{
}

ItemSearchJob::~ItemSearchJob()
{
}

void ItemSearchJob::doStart()
{
  Q_D( ItemSearchJob );

  QByteArray command = d->newTag() + " SEARCH ";
  command += ImapParser::quote( d->mQuery.toUtf8() );
  command += '\n';
  d->writeData( command );
}

void ItemSearchJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
{
  Q_D( ItemSearchJob );

  if ( tag == "*" ) {
    int begin = data.indexOf( "SEARCH" );
    if ( begin >= 0 ) {

      // split search response into key/value pairs
      QList<QByteArray> searchResponse;
      ImapParser::parseParenthesizedList( data, searchResponse, begin + 7 );

      // create a new item object
      Item::Id uid = -1;

      for ( int i = 0; i < searchResponse.count() - 1; i += 2 ) {
        const QByteArray key = searchResponse.value( i );
        const QByteArray value = searchResponse.value( i + 1 );

        if ( key == "UID" )
          uid = value.toLongLong();
      }

      if ( uid < 0 ) {
        kWarning() << "Broken search response: UID missing!";
        return;
      }

      Item item( uid );
      if ( !item.isValid() )
        return;

      d->mItems.append( item );
      return;
    }
  }
}

Item::List ItemSearchJob::items() const
{
  Q_D( const ItemSearchJob );

  return d->mItems;
}

#include "itemsearchjob.moc"
