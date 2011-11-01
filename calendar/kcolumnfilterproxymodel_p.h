/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,

    author Bertjan Broeksema <broeksema@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef AKONADI_KCOLUMNFILTERPROXYMODEL_H
#define AKONADI_KCOLUMNFILTERPROXYMODEL_H


#include <QtGui/QSortFilterProxyModel>

template <class T> class QVector;

namespace Akonadi
{

class KColumnFilterProxyModelPrivate;

/**
  Filter model to make only certain columns of a model visible. By default all
  columns are visible.
 */
class KColumnFilterProxyModel : public QSortFilterProxyModel
{
public:
  explicit KColumnFilterProxyModel( QObject* parent = 0 );
  virtual ~KColumnFilterProxyModel();

  /**
    Returns a vector containing the visible columns. If the vector is empy, all
    columns are visible.
  */
  QVector<int> visbileColumns() const;

  /**
    Convenience function. Has the same effect as:

    @code
    setVisibleColumns( QVector<int>() << column );
    @endcode

    @see setVisbileColumns
   */
  void setVisibleColumn( int column );

  /**
    Change the visible columns. Pass an empty vector to make all columns visible.
   */
  void setVisibleColumns( const QVector<int> &visibleColumns );

protected:
  virtual bool filterAcceptsColumn( int column, const QModelIndex& parent ) const;

private:
  KColumnFilterProxyModelPrivate * const d_ptr;
  Q_DECLARE_PRIVATE( KColumnFilterProxyModel )
};

}

#endif // KCOLUMNFILTERPROXYMODEL_H
