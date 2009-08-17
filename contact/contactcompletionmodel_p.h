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

#ifndef AKONADI_CONTACTCOMPLETIONMODEL_H
#define AKONADI_CONTACTCOMPLETIONMODEL_H

#include <akonadi/entitytreemodel.h>

namespace Akonadi {

class ContactCompletionModel : public EntityTreeModel
{
  Q_OBJECT

  public:
    enum Columns
    {
      NameColumn,         ///< The name of the contact.
      NameAndEmailColumn, ///< The name and the email of the contact.
      EmailColumn         ///< The preferred email of the contact.
    };

    ContactCompletionModel( Session *session, Monitor *monitor, QObject *parent = 0 );
    virtual ~ContactCompletionModel();

    virtual QVariant getData( const Item &item, int column, int role = Qt::DisplayRole ) const;
    virtual int columnCount( const QModelIndex &parent ) const;
    virtual int getColumnCount( int ) const;

    static QAbstractItemModel* self();

  private:
    static QAbstractItemModel* mSelf;
};

}

#endif
