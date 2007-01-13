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

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <QtCore/QString>
#include <QtCore/QStringList>

/**
 * This class encapsulates solving nexted boolean constructs
 */
class InterpreterItem
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
    InterpreterItem( const QString &key, const QString &comparator, const QString &pattern );

    /**
     * Creates a new solver item which combines the given child items by a given relation.
     *
     * Ownership of the child items is transfered to the item.
     */
    InterpreterItem( Relation relation );

    /**
     * Destroys the solver item and all its child items.
     */
    virtual ~InterpreterItem();

    virtual void setChildItems( const QList<InterpreterItem*> items );

    QList<InterpreterItem*> childItems() const;

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
    QList<InterpreterItem*> mItems;

    bool mIsLeaf;
};

class Parser
{
  public:
    InterpreterItem* parse( const QString &query ) const;

  private:
    QStringList tokenize( const QString& ) const;
    QStringListIterator balanced( QStringListIterator ) const;

    InterpreterItem* parseInternal( QStringListIterator &it ) const;
};

#endif
