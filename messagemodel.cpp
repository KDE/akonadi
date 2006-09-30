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

#include "message.h"
#include "messagefetchjob.h"
#include "messagemodel.h"
#include "messagequery.h"
#include "monitor.h"

#include <kmime_message.h>

#include <kdebug.h>
#include <klocale.h>

#include <QDebug>

using namespace Akonadi;

class Akonadi::MessageModel::Private
{
  public:
    QList<Message*> messages;
    QString path;
    Monitor *monitor;
};

MessageModel::MessageModel( QObject *parent ) :
    ItemModel( parent ),
    d( new Private() )
{
  d->monitor = 0;
}

MessageModel::~MessageModel( )
{
  delete d->monitor;
  delete d;
}

int MessageModel::columnCount( const QModelIndex & parent ) const
{
  Q_UNUSED( parent );
  return 5; // keep in sync with the column type enum
}

QVariant MessageModel::data( const QModelIndex & index, int role ) const
{
  if ( !index.isValid() )
    return QVariant();
  if ( index.row() >= d->messages.count() )
    return QVariant();
  Message* msg = d->messages.at( index.row() );
  Q_ASSERT( msg->mime() );
  if ( role == Qt::DisplayRole ) {
    switch ( index.column() ) {
      case Subject:
        return msg->mime()->subject()->asUnicodeString();
      case Sender:
        return msg->mime()->from()->asUnicodeString();
      case Receiver:
        return msg->mime()->to()->asUnicodeString();
      case Date:
        return msg->mime()->date()->asUnicodeString();
      case Size:
      // TODO
      default:
        return QVariant();
    }
  }
  return QVariant();
}

int MessageModel::rowCount( const QModelIndex & parent ) const
{
  if ( !parent.isValid() )
    return d->messages.count();
  return 0;
}

QVariant MessageModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
    switch ( section ) {
      case Subject:
        return i18n( "Subject" );
      case Sender:
        return i18n( "Sender" );
      case Receiver:
        return i18n( "Receiver" );
      case Date:
        return i18n( "Date" );
      case Size:
        return i18n( "Size" );
      default:
        return QString();
    }
  }
  return QAbstractTableModel::headerData( section, orientation, role );
}

ItemFetchJob* MessageModel::createFetchJob( const QString &path, QObject* parent )
{
  return new MessageFetchJob( path, parent );
}

#include "messagemodel.moc"
