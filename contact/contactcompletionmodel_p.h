/*
    This file is part of Akonadi Contact.

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

#ifndef AKONADI_CONTACTCOMPLETIONMODEL_P_H
#define AKONADI_CONTACTCOMPLETIONMODEL_P_H

#include <akonadi/entitytreemodel.h>

namespace Akonadi {

class ContactCompletionModel : public EntityTreeModel
{
  Q_OBJECT

  public:
    enum Columns {
      NameColumn,         ///< The name of the contact.
      NameAndEmailColumn, ///< The name and the email of the contact.
      EmailColumn         ///< The preferred email of the contact.
    };

    explicit ContactCompletionModel( ChangeRecorder *monitor, QObject *parent = 0 );
    virtual ~ContactCompletionModel();

    virtual QVariant entityData( const Item &item, int column, int role = Qt::DisplayRole ) const;
    virtual QVariant entityData( const Collection &collection, int column, int role = Qt::DisplayRole ) const;
    virtual int columnCount( const QModelIndex &parent ) const;
    virtual int entityColumnCount( HeaderGroup ) const;

    static QAbstractItemModel* self();

  private:
    static QAbstractItemModel* mSelf;
};

}

#endif
