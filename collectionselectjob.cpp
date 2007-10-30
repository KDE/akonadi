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

#include "collectionselectjob.h"

#include <QtCore/QDebug>

using namespace Akonadi;

class Akonadi::CollectionSelectJobPrivate
{
  public:
    CollectionSelectJobPrivate() :
      unseen( -1 ),
      silent( true )
    {
    }

    Collection collection;
    int unseen;
    bool silent;
};

CollectionSelectJob::CollectionSelectJob( const Collection &collection, QObject *parent ) :
    Job( parent ),
    d( new CollectionSelectJobPrivate )
{
  d->collection = collection;
}

CollectionSelectJob::~ CollectionSelectJob( )
{
  delete d;
}

void CollectionSelectJob::doStart( )
{
  QByteArray command( newTag() + " SELECT " );
  if ( d->silent )
    command += "SILENT ";
  writeData( command + QByteArray::number( d->collection.id() ) + '\n' );
}

void CollectionSelectJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
{
  if ( tag == "*" ) {
    if ( data.startsWith( "OK [UNSEEN" ) ) {
      int begin = data.indexOf( ' ', 4 );
      int end = data.indexOf( ']' );
      QByteArray number = data.mid( begin + 1, end - begin - 1 );
      d->unseen = number.toInt();
      return;
    }
  }
}

int CollectionSelectJob::unseen( ) const
{
  return d->unseen;
}

void CollectionSelectJob::setRetrieveStatus(bool status)
{
  d->silent = !status;
}


#include "collectionselectjob.moc"
