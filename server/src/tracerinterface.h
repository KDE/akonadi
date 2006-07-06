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

#ifndef AKONADI_TRACERINTERFACE_H
#define AKONADI_TRACERINTERFACE_H

namespace Akonadi {

/**
 * This interface can be reimplemented to deliver tracing information
 * of the akonadi server to the outside.
 *
 * Possible implementations:
 *   - log file
 *   - dbus signals
 *   - live gui
 */
class TracerInterface
{
  public:
    virtual ~TracerInterface() {}

    /**
     * This method is called whenever a new data (imap) connection to the akonadi server
     * is established.
     *
     * @param identifier The unique identifier for this connection. All input and output
     *                   messages for this connection will have the same identifier.
     *
     * @param msg A message specific string.
     */
    virtual void beginConnection( const QString &identifier, const QString &msg ) = 0;

    /**
     * This method is called whenever a data (imap) connection to akonadi server is
     * closed.
     *
     * @param identifier The unique identifier of this connection.
     * @param msg A message specific string.
     */
    virtual void endConnection( const QString &identifier, const QString &msg ) = 0;

    /**
     * This method is called whenever the akonadi server retrieves some data from the
     * outside.
     *
     * @param identifier The unique identifier of the connection on which the data
     *                   is retrieved.
     * @param msg A message specific string.
     */
    virtual void connectionInput( const QString &identifier, const QString &msg ) = 0;

    /**
     * This method is called whenever the akonadi server sends some data out to a client.
     *
     * @param identifier The unique identifier of the connection on which the
     *                   data is send.
     * @param msg A message specific string.
     */
    virtual void connectionOutput( const QString &identifier, const QString &msg ) = 0;

    /**
     * This method is called whenever a dbus signal is emitted on the bus.
     *
     * @param signalName The name of the signal being sent.
     * @param msg A message specific string.
     */
    virtual void signal( const QString &signalName, const QString &msg ) = 0;

    /**
     * This method is called whenever a component wants to output a warning.
     */
    virtual void warning( const QString &componentName, const QString &msg ) = 0;

    /**
     * This method is called whenever a component wants to output an error.
     */
    virtual void error( const QString &componentName, const QString &msg ) = 0;
};

}

#endif
