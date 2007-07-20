/*
    Copyright (c) 2007 Bruno Virlet <bruno.virlet@gmail.com>

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

#ifndef AKONADI_MESSAGETHREADERPROXYMODEL_H
#define AKONADI_MESSAGETHREADERPROXYMODEL_H

#include <libakonadi/akonadi_export.h>
#include <QtGui/QSortFilterProxyModel>

class QModelIndex;

namespace Akonadi {

class Collection;

/**
 * Proxy to thread message using the Mailthreader agent
*/
class AKONADI_KMIME_EXPORT MessageThreaderProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT

  public:
    /**
     * Create a new MessageThreaderProxyModel
     * @param parent The parent object
     */
    MessageThreaderProxyModel( QObject *parent = 0 );

    /**
     * Destroy the model
     **/
    virtual ~MessageThreaderProxyModel();

    /**
     * Reimplemented to actually do the threading.
     */
    QModelIndex parent ( const QModelIndex & index ) const;

     void setSourceModel( QAbstractItemModel *model );
  protected:

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void setCollection( Collection &col ) )
};

}

#endif
