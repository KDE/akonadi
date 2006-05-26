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

#include "collectionselectjob.h"

#include <QDebug>

using namespace PIM;

class PIM::CollectionSelectJobPrivate
{
  public:
    QByteArray path;
    QByteArray tag;
    int unseen;
};

PIM::CollectionSelectJob::CollectionSelectJob( const QByteArray & path, QObject *parent ) :
    Job( parent ),
    d( new CollectionSelectJobPrivate )
{
  d->path = path;
  d->unseen = -1;
}

PIM::CollectionSelectJob::~ CollectionSelectJob( )
{
  delete d;
}

void PIM::CollectionSelectJob::doStart( )
{
  d->tag = newTag();
  writeData( d->tag + " SELECT " + d->path );
}

void PIM::CollectionSelectJob::handleResponse( const QByteArray & tag, const QByteArray & data )
{
  if ( tag == d->tag ) {
    if ( !data.startsWith( "OK" ) )
      setError( Unknown );
    emit done( this );
    return;
  }
  if ( tag == "*" ) {
    if ( data.startsWith( "OK [UNSEEN" ) ) {
      int begin = data.indexOf( ' ', 4 );
      int end = data.indexOf( ']' );
      QByteArray number = data.mid( begin + 1, end - begin - 1 );
      d->unseen = number.toInt();
      qDebug() << "unseen items " << d->unseen << " in folder " << d->path;
      return;
    }
  }
  qDebug() << "Unhandled response in collection selection job: " << tag << data;
}

int PIM::CollectionSelectJob::unseen( ) const
{
  return d->unseen;
}


#include "collectionselectjob.moc"
