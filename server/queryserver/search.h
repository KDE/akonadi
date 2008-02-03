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

#ifndef SEARCH_H
#define SEARCH_H

#include <QtCore/QObject>

class Search : public QObject
{
  Q_OBJECT

  public:
    /**
     * Creates a new search object with the given @p parent.
     */
    Search( QObject *parent = 0 );

    /**
     * Destroys the search object.
     */
    ~Search();

  public Q_SLOTS:
    /**
     * Creates a new query object with the given @p queryString
     * and returns the dbus path to the object.
     */
    QString createQuery( const QString &queryString );

  private:
    class Private;
    Private* const d;
};

#endif
