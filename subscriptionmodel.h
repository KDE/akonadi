/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_SUBSCRIPTIONMODEL_H
#define AKONADI_SUBSCRIPTIONMODEL_H

#include <libakonadi/collection.h>
#include <libakonadi/collectionmodel.h>

namespace Akonadi {

/**
  An extended collection model used for the subscription dialog.

  @internal
*/
class SubscriptionModel : public CollectionModel
{
  Q_OBJECT
  public:
    /** Additional roles. */
    enum SubscriptionModelRoles {
      SubscriptionChangedRole = CollectionViewUserRole ///< Indicate the subscription status has been changed.
    };

    /**
      Create a new subscription model.
      @param parent the parent object
    */
    explicit SubscriptionModel( QObject *parent = 0 );

    /**
      Destructor.
    */
    ~SubscriptionModel();

    QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    Qt::ItemFlags flags( const QModelIndex &index ) const;
    bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );

    Collection::List subscribed() const;
    Collection::List unsubscribed() const;

  signals:
    /**
      Emitted when the collection model is fully loaded.
    */
    void loaded();

  private:
    class Private;
    Private* const d;
    Q_PRIVATE_SLOT( d, void listResult(KJob*) )
};

}

#endif
