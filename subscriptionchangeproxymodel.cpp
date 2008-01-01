/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "subscriptionchangeproxymodel.h"
#include "subscriptionmodel.h"

#include <kdebug.h>
#include <klocale.h>

using namespace Akonadi;

class SubscriptionChangeProxyModel::Private
{
  public:
    Qt::CheckState subscribed;
};

SubscriptionChangeProxyModel::SubscriptionChangeProxyModel(bool subscribed, QObject * parent) :
    QSortFilterProxyModel( parent ),
    d( new Private )
{
  d->subscribed = subscribed ? Qt::Checked : Qt::Unchecked;
  setDynamicSortFilter( true );
}

SubscriptionChangeProxyModel::~ SubscriptionChangeProxyModel()
{
  delete d;
}

bool SubscriptionChangeProxyModel::filterAcceptsRow(int row, const QModelIndex & parent) const
{
  QModelIndex index = sourceModel()->index( row, 0, parent );
  if ( !index.data( SubscriptionModel::SubscriptionChangedRole ).toBool() )
    return false;
  if ( index.data( Qt::CheckStateRole ) != d->subscribed )
    return false;
  return true;
}

QVariant SubscriptionChangeProxyModel::data(const QModelIndex & index, int role) const
{
  if ( role == Qt::CheckStateRole )
    return QVariant();
  return QSortFilterProxyModel::data( index, role );
}

QVariant SubscriptionChangeProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if ( section == 0 && orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
    if ( d->subscribed == Qt::Checked )
      return i18nc( "@title:column", "Subscribe To" );
    else
      return i18nc( "@title:column", "Unsubscribe From" );
  }
  return QSortFilterProxyModel::headerData( section, orientation, role );
}

#include "subscriptionchangeproxymodel.moc"
