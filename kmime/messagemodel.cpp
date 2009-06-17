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

#include "messagemodel.h"
#include "messageparts.h"

#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>

#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>
typedef boost::shared_ptr<KMime::Message> MessagePtr;

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kio/job.h>

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
  fetchScope().fetchPayloadPart( MessagePart::Envelope );
}

MessageModel::~MessageModel( )
{
  delete d;
}

QStringList MessageModel::mimeTypes() const
{
  return QStringList()
      << QLatin1String("text/uri-list")
      << QLatin1String("message/rfc822");
}

int MessageModel::rowCount( const QModelIndex & parent ) const
{
  if ( collection().isValid()
          && !collection().contentMimeTypes().contains( QLatin1String("message/rfc822") )
          && collection().contentMimeTypes() != QStringList( QLatin1String("inode/directory") ) )
    return 1;

  return ItemModel::rowCount();
}

int MessageModel::columnCount( const QModelIndex & parent ) const
{
  if ( collection().isValid()
          && !collection().contentMimeTypes().contains( QLatin1String("message/rfc822") )
          && collection().contentMimeTypes() != QStringList( QLatin1String("inode/directory") ) )
    return 1;

  if ( !parent.isValid() )
    return 5; // keep in sync with the column type enum

  return 0;
}

QVariant MessageModel::data( const QModelIndex & index, int role ) const
{
  if ( !index.isValid() )
    return QVariant();
  if ( index.row() >= rowCount() )
    return QVariant();

  if ( !collection().contentMimeTypes().contains( QLatin1String("message/rfc822") ) ) {
     if ( role == Qt::DisplayRole )
       // FIXME i18n when we unfreeze for 4.4
       return QString::fromLatin1( "This model can only handle email folders. The current collection holds mimetypes: %1").arg(
                       collection().contentMimeTypes().join( QLatin1String(",") ) );
     else
       return QVariant();
  }

  Item item = itemForIndex( index );
  if ( !item.hasPayload<MessagePtr>() )
    return QVariant();
  MessagePtr msg = item.payload<MessagePtr>();
  if ( role == Qt::DisplayRole ) {
    switch ( index.column() ) {
      case Subject:
        return msg->subject()->asUnicodeString();
      case Sender:
        return msg->from()->asUnicodeString();
      case Receiver:
        return msg->to()->asUnicodeString();
      case Date:
        return KGlobal::locale()->formatDateTime( msg->date()->dateTime().toLocalZone(), KLocale::FancyLongDate );
      case Size:
        if ( item.size() == 0 )
          return i18nc( "No size available", "-" );
        else
          return KIO::convertSize( KIO::filesize_t( item.size() ) );
      default:
        return QVariant();
    }
  } else if ( role == Qt::EditRole ) {
    switch ( index.column() ) {
      case Subject:
        return msg->subject()->asUnicodeString();
      case Sender:
        return msg->from()->asUnicodeString();
      case Receiver:
        return msg->to()->asUnicodeString();
      case Date:
        return msg->date()->dateTime().dateTime();
      case Size:
        return item.size();
      default:
        return QVariant();
    }
  }
  return ItemModel::data( index, role );
}

QVariant MessageModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( !collection().contentMimeTypes().contains( QLatin1String("message/rfc822") ) ) {
    return QVariant();
  }

  if ( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
    switch ( section ) {
      case Subject:
        return i18nc( "@title:column, message (e.g. email) subject", "Subject" );
      case Sender:
        return i18nc( "@title:column, sender of message (e.g. email)", "Sender" );
      case Receiver:
        return i18nc( "@title:column, receiver of message (e.g. email)", "Receiver" );
      case Date:
        return i18nc( "@title:column, message (e.g. email) timestamp", "Date" );
      case Size:
        return i18nc( "@title:column, message (e.g. email) size", "Size" );
      default:
        return QString();
    }
  }
  return ItemModel::headerData( section, orientation, role );
}

#include "messagemodel.moc"
