/*
  Copyright (c) 2009 KDAB
  Author: Frank Osterfeld <frank@kdab.net>

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

#ifndef AKONADI_COLLECTIONSELECTION_H
#define AKONADI_COLLECTIONSELECTION_H

#include <akonadi/collection.h>
#include <QObject>

class QItemSelection;
class QItemSelectionModel;

namespace Akonadi {

class CollectionSelection : public QObject
{
  Q_OBJECT
  public:
    explicit CollectionSelection( QItemSelectionModel *selectionModel, QObject *parent = 0 );
    ~CollectionSelection();

    QItemSelectionModel *model() const;
    Akonadi::Collection::List selectedCollections() const;
    QList<Akonadi::Collection::Id> selectedCollectionIds() const;
    bool contains( const Akonadi::Collection &c ) const;
    bool contains( const Akonadi::Collection::Id &id ) const;

    bool hasSelection() const;

  Q_SIGNALS:
    void selectionChanged( const Akonadi::Collection::List &selected,
                           const Akonadi::Collection::List &deselected );
    void collectionDeselected( const Akonadi::Collection & );
    void collectionSelected( const Akonadi::Collection & );

  private Q_SLOTS:
    void slotSelectionChanged( const QItemSelection &, const QItemSelection & );

  private:
    class Private;
    Private *const d;
};

}

#endif
