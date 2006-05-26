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

#include "collectionrenamejob.h"

#include <QDebug>

using namespace PIM;

class PIM::CollectionRenameJobPrivate
{
  public:
    QByteArray from;
    QByteArray to;
    QByteArray tag;
};

PIM::CollectionRenameJob::CollectionRenameJob( const QByteArray & from, const QByteArray & to, QObject * parent ) :
    Job( parent ),
    d( new CollectionRenameJobPrivate )
{
  d->from = from;
  d->to = to;
}

PIM::CollectionRenameJob::~ CollectionRenameJob( )
{
  delete d;
}

void PIM::CollectionRenameJob::doStart( )
{
  qDebug() << "renaming!";
  d->tag = newTag();
  writeData( d->tag + " RENAME " + d->from + " " + d->to );
}

void PIM::CollectionRenameJob::handleResponse( const QByteArray & tag, const QByteArray & data )
{
  if ( tag == d->tag ) {
    if ( !data.startsWith( "OK" ) )
      setError( Unknown );
    emit done( this );
    return;
  }
  qDebug() << "Unhandled response in collection rename job: " << tag << data;
}

#include "collectionrenamejob.moc"
