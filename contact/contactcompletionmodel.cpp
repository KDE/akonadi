/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "contactcompletionmodel_p.h"

#include <akonadi/descendantsproxymodel.h>
#include <akonadi/entityfilterproxymodel.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>

#include <kabc/addressee.h>

using namespace Akonadi;

QAbstractItemModel* ContactCompletionModel::mSelf = 0;

QAbstractItemModel* ContactCompletionModel::self()
{
  if ( mSelf )
    return mSelf;

  Monitor *monitor = new Monitor;
  monitor->fetchCollection( true );
  monitor->itemFetchScope().fetchFullPayload();
  monitor->setCollectionMonitored( Akonadi::Collection::root() );
  monitor->setMimeTypeMonitored( KABC::Addressee::mimeType() );

  ContactCompletionModel *model = new ContactCompletionModel( Session::defaultSession(), monitor );

  Akonadi::DescendantsProxyModel *descModel = new DescendantsProxyModel( model );
  descModel->setSourceModel( model );

  EntityFilterProxyModel *filter = new Akonadi::EntityFilterProxyModel( model );
  filter->setSourceModel( descModel );
  filter->addMimeTypeExclusionFilter( Akonadi::Collection::mimeType() );
  filter->setHeaderSet( Akonadi::EntityTreeModel::ItemListHeaders );

  mSelf = filter;

  return mSelf;
}

ContactCompletionModel::ContactCompletionModel( Session *session, Monitor *monitor, QObject *parent )
  : EntityTreeModel( session, monitor, parent )
{
}

ContactCompletionModel::~ContactCompletionModel()
{
}

QVariant ContactCompletionModel::getData( const Item &item, int column, int role ) const
{
  if ( !item.hasPayload<KABC::Addressee>() ) {
    // Pass modeltest
    if ( role == Qt::DisplayRole )
      return item.remoteId();

    return QVariant();
  }

  if ( role == Qt::DisplayRole || role == Qt::EditRole ) {
    const KABC::Addressee contact = item.payload<KABC::Addressee>();

    switch ( column ) {
      case NameColumn:
        if ( !contact.formattedName().isEmpty() )
          return contact.formattedName();
        else
          return contact.assembledName();
        break;
      case NameAndEmailColumn:
        return contact.fullEmail();
        break;
      case EmailColumn:
        return contact.preferredEmail();
        break;
    }
  }

  return EntityTreeModel::getData( item, column, role );
}

int ContactCompletionModel::columnCount( const QModelIndex &parent ) const
{
  if ( !parent.isValid() )
    return 3;
  else
    return 0;
}

int ContactCompletionModel::getColumnCount( int ) const
{
  return 3;
}

#include "contactcompletionmodel_p.moc"
