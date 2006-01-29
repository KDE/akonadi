/*
    This file is part of libakonadi.

    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>
                  2006 Marc Mutz <mutz@kde.org>

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

#include <QEventLoop>
#include <QHash>

#include "job.h"

using namespace PIM;

DataReference::DataReference()
{
}

DataReference::DataReference( const QString &persistanceID, const QString &externalUrl )
  : mPersistanceID( persistanceID ), mExternalUrl( externalUrl )
{
}

DataReference::~DataReference()
{
}

QString DataReference::persistanceID() const
{
  return mPersistanceID;
}

QUrl DataReference::externalUrl() const
{
  return mExternalUrl;
}

bool PIM::DataReference::isNull() const
{
  return mPersistanceID.isEmpty();
}

bool DataReference::operator==( const DataReference & other ) const
{
  return mPersistanceID == other.mPersistanceID;
}

bool PIM::DataReference::operator !=( const DataReference & other ) const
{
  return mPersistanceID != other.mPersistanceID;
}

bool DataReference::operator<( const DataReference & other ) const
{
  return mPersistanceID < other.mPersistanceID;
}

uint qHash( const DataReference& ref )
{
  return qHash( ref.persistanceID() );
}


class Job::JobPrivate
{
  public:
    int mError;
};

Job::Job( QObject *parent )
  : QObject( parent ), d( new JobPrivate )
{
  d->mError = 0;
}

Job::~Job()
{
  delete d;
  d = 0;
}

bool Job::exec()
{
  QEventLoop loop( this );
  connect( this, SIGNAL( done( PIM::Job* ) ), &loop, SLOT( quit() ) );
  doStart();
  loop.exec();

  return ( d->mError == 0 );
}

void Job::start()
{
  doStart();
}

void Job::kill()
{
  d->mError = UserCanceled;
  emit done( this );
}

int Job::error() const
{
  return d->mError;
}

QString Job::errorText() const
{
  switch ( d->mError ) {
    case None:
      return QString();
      break;
    case ConnectionFailed:
      return tr( "Can't connect to pim storage service." );
      break;
    case UserCanceled:
      return tr( "User canceled transmission." );
      break;
    case Unknown:
    default:
      return tr( "Unknown Error." );
      break;
  }
}

void Job::setError( int error )
{
  d->mError = error;
}

#include "job.moc"
