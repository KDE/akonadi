/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_MESSAGEMODEL_H
#define AKONADI_MESSAGEMODEL_H

#include "akonadi-kmime_export.h"
#include <akonadi/itemmodel.h>
#include <akonadi/job.h>

namespace Akonadi {

/**
  A flat self-updating message model.
*/
class AKONADI_KMIME_EXPORT MessageModel : public Akonadi::ItemModel
{
  Q_OBJECT

  public:
    /**
      Column types.
    */
    enum Column {
      Subject, /**< Subject column. */
      Sender, /**< Sender column. */
      Receiver, /**< Receiver column. */
      Date, /**< Date column. */
      Size /**< Size column. */
    };

    /**
      Creates a new message model.

      @param parent The parent object.
    */
    explicit MessageModel( QObject* parent = 0 );

    /**
      Deletes the message model.
    */
    virtual ~MessageModel();

    /**
      Reimplemented from QAbstractItemModel.
     */
    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const;

    /**
      Reimplemented from QAbstractItemModel.
     */
    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const;

    /**
      Reimplemented from QAbstractItemModel.
     */
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;

    /**
      Reimplemented from QAbstractItemModel.
     */
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    /**
      Reimplemented from QAbstractItemModel.
     */
    virtual QStringList mimeTypes() const;
  private:
    class Private;
    Private* const d;
};

}

#endif
