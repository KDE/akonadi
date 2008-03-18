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

#ifndef AKONADI_SUBSCRIPTIONCHANGEPROXYMODEL_H
#define AKONADI_SUBSCRIPTIONCHANGEPROXYMODEL_H

#include <QtGui/QSortFilterProxyModel>

namespace Akonadi {

/**
  Proxy model that can be used on top of a SubscriptionModel to filter out
  only the actual changes.

  @internal
*/
class SubscriptionChangeProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
  public:
    /**
      Create a new subscription change proxy model.
      @param subscribed @c true to list only newly subscribed collections, @c false to
      list only newly unsubscribed collections.
      @param parent The parent object.
    */
    explicit SubscriptionChangeProxyModel( bool subscribed, QObject *parent = 0 );

    /**
      Destructor.
    */
    ~SubscriptionChangeProxyModel();

    QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

  protected:
    bool filterAcceptsRow( int row, const QModelIndex &parent ) const;

  private:
    class Private;
    Private* const d;
};

}

#endif
