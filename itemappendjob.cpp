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

#include "itemappendjob.h"

#include <QDebug>

using namespace PIM;

class PIM::ItemAppendJobPrivate
{
  public:
    QByteArray path;
    QByteArray data;
    QByteArray mimetype;
    QByteArray tag;
};

PIM::ItemAppendJob::ItemAppendJob( const QByteArray &path, const QByteArray & data, const QByteArray & mimetype, QObject * parent ) :
    Job( parent ),
    d( new ItemAppendJobPrivate )
{
  d->path = path;
  d->data = data;
  d->mimetype = mimetype;
}

PIM::ItemAppendJob::~ ItemAppendJob( )
{
  delete d;
}

void PIM::ItemAppendJob::doStart()
{
  d->tag = newTag();
  writeData( d->tag + " APPEND " + d->path + " (\\MimeType[" + d->mimetype + "]) {" + QByteArray::number( d->data.size() ) + "}" );
}

void PIM::ItemAppendJob::handleResponse( const QByteArray & tag, const QByteArray & data )
{
  if ( tag == d->tag ) {
    if ( !data.startsWith( "OK" ) )
      setError( Unknown );
    emit done( this );
    return;
  }
  if ( tag == "+" ) { // ready for literal data
    writeData( d->data );
    return;
  }
  qDebug() << "unhandled response in item append job: " << tag << data;
}


#include "itemappendjob.moc"
