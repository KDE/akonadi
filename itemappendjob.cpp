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

#include <QtCore/QDebug>

using namespace Akonadi;

class Akonadi::ItemAppendJobPrivate
{
  public:
    QString path;
    QByteArray data;
    QByteArray mimetype;
    QString remoteId;
};

ItemAppendJob::ItemAppendJob( const QString &path, const QByteArray & data, const QByteArray & mimetype, QObject * parent ) :
    Job( parent ),
    d( new ItemAppendJobPrivate )
{
  d->path = path;
  d->data = data;
  d->mimetype = mimetype;
}

ItemAppendJob::~ ItemAppendJob( )
{
  delete d;
}

void ItemAppendJob::doStart()
{
  QByteArray remoteId;
  if ( !d->remoteId.isEmpty() )
    remoteId = " \\RemoteId[" + d->remoteId.toUtf8() + ']';
  writeData( newTag() + " APPEND \"" + d->path.toUtf8() + "\" (\\MimeType[" + d->mimetype + ']' + remoteId + ") {" + QByteArray::number( d->data.size() ) + '}' );
}

void ItemAppendJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
{
  if ( tag == "+" ) { // ready for literal data
    writeData( d->data );
    return;
  }
  qDebug() << "unhandled response in item append job: " << tag << data;
}

void ItemAppendJob::setRemoteId(const QString & remoteId)
{
  d->remoteId = remoteId;
}

#include "itemappendjob.moc"
