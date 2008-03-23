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

#include "job_p.h"

#include <QtCore/QDebug>

using namespace Akonadi;

class Akonadi::CollectionSelectJobPrivate : public JobPrivate
{
  public:
    CollectionSelectJobPrivate( CollectionSelectJob *parent )
      : JobPrivate( parent ),
        mUnseen( -1 ),
        mSilent( true )
    {
    }

    Collection mCollection;
    int mUnseen;
    bool mSilent;
};

CollectionSelectJob::CollectionSelectJob( const Collection &collection, QObject *parent )
  : Job( new CollectionSelectJobPrivate( this ), parent )
{
  Q_D( CollectionSelectJob );

  d->mCollection = collection;
}

CollectionSelectJob::~CollectionSelectJob( )
{
}

void CollectionSelectJob::doStart( )
{
  Q_D( CollectionSelectJob );

  QByteArray command( newTag() + " SELECT " );
  if ( d->mSilent )
    command += "SILENT ";
  writeData( command + QByteArray::number( d->mCollection.id() ) + '\n' );
}

void CollectionSelectJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
{
  Q_D( CollectionSelectJob );

  if ( tag == "*" ) {
    if ( data.startsWith( "OK [UNSEEN" ) ) {
      int begin = data.indexOf( ' ', 4 );
      int end = data.indexOf( ']' );
      QByteArray number = data.mid( begin + 1, end - begin - 1 );
      d->mUnseen = number.toInt();
      return;
    }
  }
}

int CollectionSelectJob::unseen( ) const
{
  Q_D( const CollectionSelectJob );

  return d->mUnseen;
}

void CollectionSelectJob::setRetrieveStatus(bool status)
{
  Q_D( CollectionSelectJob );

  d->mSilent = !status;
}


#include "collectionselectjob.moc"
