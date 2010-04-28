
#include "emailaddressselectionproxymodel_p.h"

#include <akonadi/item.h>
#include <kabc/addressee.h>
#include <kabc/contactgroup.h>
#include <klocale.h>

using namespace Akonadi;

EmailAddressSelectionProxyModel::EmailAddressSelectionProxyModel( QObject *parent )
  : LeafExtensionProxyModel( parent )
{
}

EmailAddressSelectionProxyModel::~EmailAddressSelectionProxyModel()
{
}

QVariant EmailAddressSelectionProxyModel::data( const QModelIndex &index, int role ) const
{
  const QVariant value = LeafExtensionProxyModel::data( index, role );

  if ( !value.isValid() ) { // index is not a leaf child
    if ( role == NameRole ) {
      const Akonadi::Item item = index.data( ContactsTreeModel::ItemRole ).value<Akonadi::Item>();
      if ( item.hasPayload<KABC::Addressee>() ) {
        const KABC::Addressee contact = item.payload<KABC::Addressee>();
        return contact.realName();
      } else if ( item.hasPayload<KABC::ContactGroup>() ) {
        const KABC::ContactGroup group = item.payload<KABC::ContactGroup>();
        return group.name();
      }
    } else if ( role == EmailAddressRole ) {
      const Akonadi::Item item = index.data( ContactsTreeModel::ItemRole ).value<Akonadi::Item>();
      if ( item.hasPayload<KABC::Addressee>() ) {
        const KABC::Addressee contact = item.payload<KABC::Addressee>();
        return contact.preferredEmail();
      } else if ( item.hasPayload<KABC::ContactGroup>() ) {
        const KABC::ContactGroup group = item.payload<KABC::ContactGroup>();
        return group.name(); // the name must be resolved by the caller
      }
    }
  }

  return value;
}

int EmailAddressSelectionProxyModel::leafRowCount( const QModelIndex &index ) const
{
  const Akonadi::Item item = index.data( ContactsTreeModel::ItemRole ).value<Akonadi::Item>();
  if ( item.hasPayload<KABC::Addressee>() ) {
    const KABC::Addressee contact = item.payload<KABC::Addressee>();
    if ( contact.emails().count() == 1 )
      return 0;
    else
      return contact.emails().count();
  } else if ( item.hasPayload<KABC::ContactGroup>() ) {
    const KABC::ContactGroup group = item.payload<KABC::ContactGroup>();
    return group.dataCount();
  } else {
    return 0;
  }
}

int EmailAddressSelectionProxyModel::leafColumnCount( const QModelIndex &index ) const
{
  const Akonadi::Item item = index.data( ContactsTreeModel::ItemRole ).value<Akonadi::Item>();
  if ( item.hasPayload<KABC::Addressee>() )
    return 1;
  else if ( item.hasPayload<KABC::ContactGroup>() )
    return 1;
  else
    return 0;
}

QVariant EmailAddressSelectionProxyModel::leafData( const QModelIndex &index, int row, int, int role ) const
{
  if ( role == Qt::DisplayRole ) {
    const Akonadi::Item item = index.data( ContactsTreeModel::ItemRole ).value<Akonadi::Item>();
    if ( item.hasPayload<KABC::Addressee>() ) {
      const KABC::Addressee contact = item.payload<KABC::Addressee>();
      if ( row >= 0 && row < contact.emails().count() )
        return contact.emails().at( row );
    } else if ( item.hasPayload<KABC::ContactGroup>() ) {
      const KABC::ContactGroup group = item.payload<KABC::ContactGroup>();
      if ( row >= 0 && row < (int)group.dataCount() )
        return i18nc( "name <email>", "%1 <%2>", group.data( row ).name(), group.data( row ).email() );
    }
  } else if ( role == NameRole ) {
    const Akonadi::Item item = index.data( ContactsTreeModel::ItemRole ).value<Akonadi::Item>();
    if ( item.hasPayload<KABC::Addressee>() ) {
      const KABC::Addressee contact = item.payload<KABC::Addressee>();
      return contact.realName();
    } else if ( item.hasPayload<KABC::ContactGroup>() ) {
      const KABC::ContactGroup group = item.payload<KABC::ContactGroup>();
      if ( row >= 0 && row < (int)group.dataCount() )
        return group.data( row ).name();
    }
  } else if ( role == EmailAddressRole ) {
    const Akonadi::Item item = index.data( ContactsTreeModel::ItemRole ).value<Akonadi::Item>();
    if ( item.hasPayload<KABC::Addressee>() ) {
      const KABC::Addressee contact = item.payload<KABC::Addressee>();
      if ( row >= 0 && row < contact.emails().count() )
        return contact.emails().at( row );
    } else if ( item.hasPayload<KABC::ContactGroup>() ) {
      const KABC::ContactGroup group = item.payload<KABC::ContactGroup>();
      if ( row >= 0 && row < (int)group.dataCount() )
        return group.data( row ).email();
    }
  } else
    return index.data( role );

  return QVariant();
}
