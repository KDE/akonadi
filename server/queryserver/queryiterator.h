/***************************************************************************
 *   Copyright (C) 2008 by Tobias Koenig <tokoe@kde.org>                   *
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

#ifndef QUERYITERATOR_H
#define QUERYITERATOR_H

#include <QtCore/QObject>
#include <QtCore/QPair>

#include <Soprano/QueryResultIterator>

class QueryIterator : public QObject
{
  Q_OBJECT

  public:
    /**
     * Creates a new search query iterator with the given @p id and @p parent.
     */
    QueryIterator( const Soprano::QueryResultIterator &it, const QString &id, QObject *parent = 0 );

    /**
     * Destroys the search query iterator.
     */
    ~QueryIterator();

  public Q_SLOTS:
    /**
     * Returns whether there is a next hit.
     */
    bool next();

    /**
     * Returns the uri of the current hit.
     */
    QString currentUri();

    /**
     * Returns the score of the current hit.
     */
    double currentScore();

    /**
     * Closes the query iterator and destroys the dbus object.
     */
    void close();

  private:
    class Private;
    Private* const d;
};

#endif
