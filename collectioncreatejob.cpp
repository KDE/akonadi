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

#include "collectioncreatejob.h"
#include "imapparser.h"

#include <QDebug>

using namespace PIM;

class PIM::CollectionCreateJobPrivate {
  public:
    QByteArray path;
    QList<QByteArray> contentTypes;
};

PIM::CollectionCreateJob::CollectionCreateJob( const QByteArray & path, QObject * parent ) :
    Job( parent ),
    d( new CollectionCreateJobPrivate )
{
  d->path = path;
}

PIM::CollectionCreateJob::~ CollectionCreateJob( )
{
  delete d;
}

void PIM::CollectionCreateJob::doStart( )
{
  QByteArray command = newTag() + " CREATE \"" + d->path + "\"";
  if ( !d->contentTypes.isEmpty() )
    command += '(' + ImapParser::join( d->contentTypes, QByteArray(" ") ) + ')';
  writeData( command );
}

void PIM::CollectionCreateJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
{
  qDebug() << "unhandled response in collection create job: " << tag << data;
}

QByteArray PIM::CollectionCreateJob::path( ) const
{
  return d->path;
}

void PIM::CollectionCreateJob::setContentTypes(const QList< QByteArray > & contentTypes)
{
  d->contentTypes = contentTypes;
}

#include "collectioncreatejob.moc"
