/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#ifndef COLLECTIONCOMBOBOX_H
#define COLLECTIONCOMBOBOX_H

#include <libakonadi/libakonadi_export.h>
#include <QtGui/QWidget>

class QAbstractItemModel;

namespace Akonadi {

class Collection;

/**
 * This class provides a combobox for selecting a collection.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADI_COMPONENTS_EXPORT CollectionComboBox : public QWidget
{
  Q_OBJECT

  public:
    /**
     * Creates a new collection combobox.
     *
     * @param parent The parent widget.
     */
    CollectionComboBox( QWidget *parent = 0 );

    /**
     * Destroys the collection combobox.
     */
    ~CollectionComboBox();

    /**
     * Sets the collection model.
     */
    void setModel( QAbstractItemModel *model );

    /**
     * Returns the identifier of the selected collection
     */
    Collection selectedCollection() const;

  Q_SIGNALS:
    /**
     * This signal is emitted whenever the selected collection changed.
     *
     * @param identifier The identifier of the selected collection.
     */
    void selectionChanged( const Akonadi::Collection &identifier );

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void activated( int ) )
};

}

#endif
