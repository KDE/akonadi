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

#ifndef QUERY_H
#define QUERY_H

#include <QtCore/QObject>

#include <Soprano/Client/DBusModel>

class Query : public QObject
{
  Q_OBJECT

  public:
    /**
     * Creates a new search query.
     *
     * @param queryString The sparql query string.
     * @param model The soprano model.
     * @param id The dbus path id.
     * @param parent The parent object.
     */
    Query( const QString &queryString, Soprano::Model *model, const QString &id, QObject *parent = 0 );

    /**
     * Destroys the search query.
     */
    ~Query();

  public Q_SLOTS:
    /**
     * Starts the query.
     */
    void start();

    /**
     * Stops the query.
     *
     * You can restart it by calling start().
     */
    void stop();

    /**
     * Closes the query and destroys the dbus object.
     */
    void close();

    /**
     * Returns the dbus path to an iterator for all hits.
     */
    QString allHits();

  Q_SIGNALS:
    /**
     * This signal is emitted whenever hits has changed.
     */
    void hitsChanged( const QMap<QString, double> &hits );

    /**
     * This signal is emitted whenever hits has been removed.
     */
    void hitsRemoved( const QMap<QString, double> &hits );

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void _k_statementsAdded() )
    Q_PRIVATE_SLOT( d, void _k_statementsRemoved() )
};

#endif
