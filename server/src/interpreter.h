/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef AKONADI_INTERPRETER_H
#define AKONADI_INTERPRETER_H

#include <QtCore/QString>
#include <QtCore/QStringList>

namespace Akonadi {

/**
 * This class encapsulates solving nexted boolean constructs
 */
class SearchInterpreterItem
{
  public:
    enum Relation
    {
      And,
      Or
    };

    /**
     * Creates a new solver item with the given key, comparator and pattern.
     */
    SearchInterpreterItem( const QString &key, const QString &comparator, const QString &pattern );

    /**
     * Creates a new solver item which combines the given child items by a given relation.
     *
     * Ownership of the child items is transfered to the item.
     */
    SearchInterpreterItem( Relation relation );

    /**
     * Destroys the solver item and all its child items.
     */
    virtual ~SearchInterpreterItem();

    virtual void setChildItems( const QList<SearchInterpreterItem*> items );

    QList<SearchInterpreterItem*> childItems() const;

    bool isLeafItem() const;
    Relation relation() const;

    QString key() const;
    QString comparator() const;
    QString pattern() const;

    QString dump() const;

  private:
    QString mKey;
    QString mComparator;
    QString mPattern;

    Relation mRelation;
    QList<SearchInterpreterItem*> mItems;

    bool mIsLeaf;
};

class SearchParser
{
  public:
    SearchInterpreterItem* parse( const QString &query ) const;

  private:
    QStringList tokenize( const QString& ) const;
    QStringListIterator balanced( QStringListIterator ) const;

    SearchInterpreterItem* parseInternal( QStringListIterator &it ) const;
};

}

#endif
