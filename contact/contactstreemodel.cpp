/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "contactstreemodel.h"

#include <kabc/addressee.h>
#include <kabc/contactgroup.h>
#include <kglobal.h>
#include <kicon.h>
#include <kiconloader.h>
#include <klocale.h>
#include <klocalizedstring.h>

using namespace Akonadi;

class ContactsTreeModel::Private
{
  public:
    Private()
      : mColumns( ContactsTreeModel::Columns() << ContactsTreeModel::FullName ),
        mIconSize( KIconLoader::global()->currentSize( KIconLoader::Small ) )
    {
    }

    Columns mColumns;
    const int mIconSize;
};

ContactsTreeModel::ContactsTreeModel( ChangeRecorder *monitor, QObject *parent )
  : EntityTreeModel( monitor, parent ), d( new Private )
{
}

ContactsTreeModel::~ContactsTreeModel()
{
  delete d;
}

void ContactsTreeModel::setColumns( const Columns &columns )
{
  emit beginResetModel();
  d->mColumns = columns;
  emit endResetModel();
}

ContactsTreeModel::Columns ContactsTreeModel::columns() const
{
  return d->mColumns;
}

QVariant ContactsTreeModel::entityData( const Item &item, int column, int role ) const
{
  if ( item.mimeType() == KABC::Addressee::mimeType() ) {
    if ( !item.hasPayload<KABC::Addressee>() ) {

      // Pass modeltest
      if ( role == Qt::DisplayRole ) {
        return item.remoteId();
      }

      return QVariant();
    }

    const KABC::Addressee contact = item.payload<KABC::Addressee>();

    if ( role == Qt::DecorationRole ) {
      if ( column == 0 ) {
        const KABC::Picture picture = contact.photo();
        if ( picture.isIntern() ) {
          return picture.data().scaled( QSize( d->mIconSize, d->mIconSize ), Qt::KeepAspectRatio );
        } else {
          return KIcon( QLatin1String( "user-identity" ) );
        }
      }
      return QVariant();
    } else if ( ( role == Qt::DisplayRole ) || ( role == Qt::EditRole ) ) {
      switch ( d->mColumns.at( column ) ) {
        case FullName:
          if( contact.realName().isEmpty() ) {
            if( contact.preferredEmail().isEmpty() ) {
              return contact.familyName();
            }
            return contact.preferredEmail();
          }
          return contact.realName();
          break;
        case FamilyName:
          return contact.familyName();
          break;
        case GivenName:
          return contact.givenName();
          break;
        case Birthday:
          if ( contact.birthday().date().isValid() ) {
            return KGlobal::locale()->formatDate( contact.birthday().date(), KLocale::ShortDate );
          }
          break;
        case HomeAddress:
          {
            const KABC::Address address = contact.address( KABC::Address::Home );
            if ( !address.isEmpty() ) {
              return address.formattedAddress();
            }
          }
          break;
        case BusinessAddress:
          {
            const KABC::Address address = contact.address( KABC::Address::Work );
            if ( !address.isEmpty() ) {
              return address.formattedAddress();
            }
          }
          break;
        case PhoneNumbers:
          {
            QStringList values;

            const KABC::PhoneNumber::List numbers = contact.phoneNumbers();
            foreach ( const KABC::PhoneNumber &number, numbers ) {
              values += number.number();
            }

            return values.join( QLatin1String( "\n" ) );
          }
          break;
        case PreferredEmail:
          return contact.preferredEmail();
          break;
        case AllEmails:
          return contact.emails().join( QLatin1String( "\n" ) );
          break;
        case Organization:
          return contact.organization();
          break;
        case Role:
          return contact.role();
          break;
        case Homepage:
          return contact.url().url();
          break;
        case Note:
          return contact.note();
          break;
      }
    } else if ( role == DateRole ) {
      if ( d->mColumns.at( column ) == Birthday ) {
        return contact.birthday();
      } else {
        return QDate();
      }
    }
  } else if ( item.mimeType() == KABC::ContactGroup::mimeType() ) {
    if ( !item.hasPayload<KABC::ContactGroup>() ) {

      // Pass modeltest
      if ( role == Qt::DisplayRole ) {
        return item.remoteId();
      }

      return QVariant();
    }

    if ( role == Qt::DecorationRole ) {
      if ( column == 0 ) {
        return KIcon( QLatin1String( "x-mail-distribution-list" ) );
      } else {
        return QVariant();
      }
    } else if ( ( role == Qt::DisplayRole ) || ( role == Qt::EditRole ) ) {
      switch ( d->mColumns.at( column ) ) {
        case FullName:
          {
            const KABC::ContactGroup group = item.payload<KABC::ContactGroup>();
            return group.name();
          }
          break;
        default:
            return QVariant();
          break;
      }
    }
  }

  return EntityTreeModel::entityData( item, column, role );
}

QVariant ContactsTreeModel::entityData( const Collection &collection, int column, int role ) const
{
  if ( role == Qt::DisplayRole ) {
    switch ( column ) {
      case 0:
        return EntityTreeModel::entityData( collection, column, role );
      default:
        return QString(); // pass model test
    }
  }

  return EntityTreeModel::entityData( collection, column, role );
}

int ContactsTreeModel::entityColumnCount( HeaderGroup headerGroup ) const
{
  if ( headerGroup == EntityTreeModel::CollectionTreeHeaders ) {
    return 1;
  } else if ( headerGroup == EntityTreeModel::ItemListHeaders ) {
    return d->mColumns.count();
  } else {
    return EntityTreeModel::entityColumnCount( headerGroup );
  }
}

QVariant ContactsTreeModel::entityHeaderData( int section, Qt::Orientation orientation, int role, HeaderGroup headerGroup ) const
{
  if ( role == Qt::DisplayRole ) {
    if ( orientation == Qt::Horizontal ) {
      if ( headerGroup == EntityTreeModel::CollectionTreeHeaders ) {

        if ( section >= 1 ) {
          return QVariant();
        }

        switch ( section ) {
          case 0:
            return i18nc( "@title:column address books overview", "Address Books" );
            break;
        }
      } else if ( headerGroup == EntityTreeModel::ItemListHeaders ) {
        if ( section < 0 || section >= d->mColumns.count() ) {
          return QVariant();
        }

        switch ( d->mColumns.at( section ) ) {
          case FullName:
            return i18nc( "@title:column name of a person", "Name" );
            break;
          case FamilyName:
            return i18nc( "@title:column family name of a person", "Family Name" );
            break;
          case GivenName:
            return i18nc( "@title:column given name of a person", "Given Name" );
            break;
          case Birthday:
            return KABC::Addressee::birthdayLabel();
            break;
          case HomeAddress:
            return i18nc( "@title:column home address of a person", "Home" );
            break;
          case BusinessAddress:
            return i18nc( "@title:column work address of a person", "Work" );
            break;
          case PhoneNumbers:
            return i18nc( "@title:column phone numbers of a person", "Phone Numbers" );
            break;
          case PreferredEmail:
            return i18nc( "@title:column the preferred email addresses of a person", "Preferred EMail" );
            break;
          case AllEmails:
            return i18nc( "@title:column all email addresses of a person", "All EMails" );
            break;
          case Organization:
            return KABC::Addressee::organizationLabel();
            break;
          case Role:
            return KABC::Addressee::roleLabel();
            break;
          case Homepage:
            return KABC::Addressee::urlLabel();
            break;
          case Note:
            return KABC::Addressee::noteLabel();
            break;
        }
      }
    }
  }

  return EntityTreeModel::entityHeaderData( section, orientation, role, headerGroup );
}

