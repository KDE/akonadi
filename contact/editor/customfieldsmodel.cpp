/*
    This file is part of Akonadi Contact.

    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "customfieldsmodel.h"

#include <kglobal.h>
#include <kicon.h>
#include <klocale.h>

#include <QtCore/QDateTime>

Q_DECLARE_METATYPE( Qt::CheckState )

CustomFieldsModel::CustomFieldsModel( QObject *parent )
  : QAbstractItemModel( parent )
{
}

CustomFieldsModel::~CustomFieldsModel()
{
}

void CustomFieldsModel::setCustomFields( const CustomField::List &customFields )
{
  emit layoutAboutToBeChanged();

  mCustomFields = customFields;

  emit layoutChanged();
}

CustomField::List CustomFieldsModel::customFields() const
{
  return mCustomFields;
}

QModelIndex CustomFieldsModel::index( int row, int column, const QModelIndex& ) const
{
  return createIndex( row, column, 0 );
}

QModelIndex CustomFieldsModel::parent( const QModelIndex& ) const
{
  return QModelIndex();
}

QVariant CustomFieldsModel::data( const QModelIndex &index, int role ) const
{
  if ( !index.isValid() ) {
    return QVariant();
  }

  if ( index.row() < 0 || index.row() >= mCustomFields.count() ) {
    return QVariant();
  }

  if ( index.column() < 0 || index.column() > 2 ) {
    return QVariant();
  }

  const CustomField &customField = mCustomFields[ index.row() ];

  if ( role == Qt::DisplayRole ) {
    if ( index.column() == 0 ) {
      return customField.title();
    } else if ( index.column() == 1 ) {
      switch ( customField.type() ) {
        case CustomField::TextType:
        case CustomField::NumericType:
        case CustomField::UrlType:
          return customField.value();
          break;
        case CustomField::BooleanType:
          return QString();
          break;
        case CustomField::DateType:
          {
            const QDate value = QDate::fromString( customField.value(), Qt::ISODate );
            return KGlobal::locale()->formatDate( value, KLocale::ShortDate );
          }
          break;
        case CustomField::TimeType:
          {
            const QTime value = QTime::fromString( customField.value(), Qt::ISODate );
            return KGlobal::locale()->formatTime( value );
          }
          break;
        case CustomField::DateTimeType:
          {
            const QDateTime value = QDateTime::fromString( customField.value(), Qt::ISODate );
            return KGlobal::locale()->formatDateTime( value );
          }
          break;
      }
      return customField.value();
    } else {
      return customField.key();
    }
  }

  if ( role == Qt::CheckStateRole ) {
    if ( index.column() == 1 ) {
      if ( customField.type() == CustomField::BooleanType ) {
        return ( customField.value() == QLatin1String( "true" ) ? Qt::Checked : Qt::Unchecked );
      }
    }
  }

  if ( role == Qt::EditRole ) {
    if ( index.column() == 0 ) {
      return customField.title();
    } else if ( index.column() == 1 ) {
      return customField.value();
    } else {
      return customField.key();
    }
  }

  if ( role == TypeRole ) {
    return customField.type();
  }

  if ( role == ScopeRole ) {
    return customField.scope();
  }

  return QVariant();
}

bool CustomFieldsModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
  if ( !index.isValid() ) {
    return false;
  }

  if ( index.row() < 0 || index.row() >= mCustomFields.count() ) {
    return false;
  }

  if ( index.column() < 0 || index.column() > 2 ) {
    return false;
  }

  CustomField &customField = mCustomFields[ index.row() ];

  if ( role == Qt::EditRole ) {
    if ( index.column() == 0 ) {
      customField.setTitle( value.toString() );
    } else if ( index.column() == 1 ) {
      customField.setValue( value.toString() );
    } else {
      customField.setKey( value.toString() );
    }

    emit dataChanged( index, index );
    return true;
  }

  if ( role == Qt::CheckStateRole ) {
    if ( index.column() == 1 ) {
      if ( customField.type() == CustomField::BooleanType ) {
        customField.setValue( static_cast<Qt::CheckState>( value.toInt() ) == Qt::Checked ?
                              QLatin1String( "true" ) : QLatin1String( "false" ) );
        emit dataChanged( index, index );
        return true;
      }
    }
  }

  if ( role == TypeRole ) {
    customField.setType( (CustomField::Type)value.toInt() );
    emit dataChanged( index, index );
    return true;
  }

  if ( role == ScopeRole ) {
    customField.setScope( (CustomField::Scope)value.toInt() );
    emit dataChanged( index, index );
    return true;
  }

  return false;
}

QVariant CustomFieldsModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( section < 0 || section > 1 ) {
    return QVariant();
  }

  if ( orientation != Qt::Horizontal ) {
    return QVariant();
  }

  if ( role != Qt::DisplayRole ) {
    return QVariant();
  }

  if ( section == 0 ) {
    return i18nc( "custom field title", "Title" );
  } else {
    return i18nc( "custom field value", "Value" );
  }
}

Qt::ItemFlags CustomFieldsModel::flags( const QModelIndex &index ) const
{
  if ( !index.isValid() || index.row() < 0 || index.row() >= mCustomFields.count() ) {
    return QAbstractItemModel::flags( index );
  }

  const CustomField &customField = mCustomFields[ index.row() ];

  const Qt::ItemFlags parentFlags = QAbstractItemModel::flags( index );
  if ( ( customField.type() == CustomField::BooleanType ) && ( index.column() == 1 ) ) {
    return ( parentFlags | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsUserCheckable );
  } else {
    return ( parentFlags | Qt::ItemIsEnabled | Qt::ItemIsEditable );
  }
}

int CustomFieldsModel::columnCount( const QModelIndex &parent ) const
{
  if ( !parent.isValid() ) {
    return 3;
  } else {
    return 0;
  }
}

int CustomFieldsModel::rowCount( const QModelIndex &parent ) const
{
  if ( !parent.isValid() ) {
    return mCustomFields.count();
  } else {
    return 0;
  }
}

bool CustomFieldsModel::insertRows( int row, int count, const QModelIndex &parent )
{
  if ( parent.isValid() ) {
    return false;
  }

  beginInsertRows( parent, row, row + count - 1 );
  for ( int i = 0; i < count; ++i ) {
    mCustomFields.insert( row, CustomField() );
  }
  endInsertRows();

  return true;
}

bool CustomFieldsModel::removeRows( int row, int count, const QModelIndex &parent )
{
  if ( parent.isValid() ) {
    return false;
  }

  beginRemoveRows( parent, row, row + count - 1 );
  for ( int i = 0; i < count; ++i ) {
    mCustomFields.remove( row );
  }
  endRemoveRows();

  return true;
}

