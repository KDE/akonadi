/*
  Copyright (c) 2009 KDAB
  Author: Frank Osterfeld <frank@kdab.net>

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

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#ifndef AKONADI_COLLECTIONSELECTION_H
#define AKONADI_COLLECTIONSELECTION_H

#include <QObject>

#include <Akonadi/Collection>

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
