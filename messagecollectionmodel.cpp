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

#include "messagecollectionattribute.h"
#include "messagecollectionmodel.h"

#include <kdebug.h>
#include <klocale.h>

#include <QFont>

using namespace PIM;

PIM::MessageCollectionModel::MessageCollectionModel( QObject * parent ) :
    CollectionModel( parent )
{
}

int PIM::MessageCollectionModel::columnCount( const QModelIndex & parent ) const
{
  Q_UNUSED( parent );
  return 3;
}

QVariant PIM::MessageCollectionModel::data( const QModelIndex & index, int role ) const
{
  Collection *col = static_cast<Collection*>( index.internalPointer() );
  MessageCollectionAttribute *attr = col->attribute<MessageCollectionAttribute>();

  if ( role == Qt::DisplayRole && attr && ( index.column() == 1 || index.column() == 2 ) ) {
    int value = -1;
    switch ( index.column() ) {
      case 1: value = attr->unreadCount(); break;
      case 2: value = attr->count(); break;
    }
    if ( value < 0 )
      return QString();
    else if ( value == 0 )
      return QString( "-" );
    else
      return QString::number( value );
  }

  if ( role == Qt::TextAlignmentRole && ( index.column() == 1 || index.column() == 2 ) )
    return Qt::AlignRight;

  // ### that's wrong, we'll need a custom delegate anyway
  if ( role == Qt::FontRole && attr && attr->unreadCount() > 0 ) {
    QFont f;
    f.setBold( true );
    return f;
  }

  return CollectionModel::data( index, role );
}

QVariant PIM::MessageCollectionModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( orientation == Qt::Horizontal && role == Qt::DisplayRole )
    switch ( section ) {
      case 1: return i18n( "Unread" );
      case 2: return i18n( "Total" );
    }

  return CollectionModel::headerData( section, orientation, role );
}

#include "messagecollectionmodel.moc"
