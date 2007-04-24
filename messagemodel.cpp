/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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
#include "monitor.h"
#include "session.h"

#include <kmime/kmime_message.h>

#include <kdebug.h>
#include <klocale.h>

#include <QtCore/QDebug>

using namespace Akonadi;

class Akonadi::MessageModel::Private
{
  public:
};

MessageModel::MessageModel( QObject *parent ) :
    ItemModel( parent ),
    d( new Private() )
{
}

MessageModel::~MessageModel( )
{
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
  if ( index.row() >= rowCount() )
    return QVariant();
  Item item = itemForIndex( index );
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
  return ItemModel::headerData( section, orientation, role );
}

#include "messagemodel.moc"
