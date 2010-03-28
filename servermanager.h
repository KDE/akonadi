/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_SERVERMANAGER_H
#define AKONADI_SERVERMANAGER_H

#include "akonadi_export.h"

#include <QtCore/QObject>

namespace Akonadi {

class ServerManagerPrivate;

/**
 * @short Provides methods to control the Akonadi server process.
 *
 * Asynchronous, low-level control of the Akonadi server.
 * Akonadi::Control provides a synchronous interface to some of the methods in here.
 *
 * @author Volker Krause <vkrause@kde.org>
 * @see Akonadi::Control
 * @since 4.2
 */
class AKONADI_EXPORT ServerManager : public QObject
{
  Q_OBJECT
  public:
    /**
     * Enum for the various states the server can be in.
     * @since 4.5
     */
    enum State {
        NotRunning, ///< Server is not running, could be noone started it yet or it failed to start.
        Starting, ///< Server was started but is not yet running.
        Running, ///< Server is running and operational.
        Stopping, ///< Server is shutting down.
        Broken ///< Server is not operational and an error has been detected.
    };

    /**
     * Starts the server. This method returns imediately and does not wait
     * until the server is actually up and running.
     * @return @c true if the start was possible (which not necessarily means
     * the server is really running though) and @c false if an immediate error occurred.
     * @see Akonadi::Control::start()
     */
    static bool start();

    /**
     * Stops the server. This methods returns immediately after the shutdown
     * command has been send and does not wait until the server is actually
     * shut down.
     * @return @c true if the shutdown command was sent successfully, @c false
     * otherwise
     */
    static bool stop();

    /**
     * Shows the Akonadi self test dialog, which tests Akonadi for various problems
     * and reports these to the user if.
     * @param parent the parent widget for the dialog
     */
    static void showSelfTestDialog( QWidget *parent );

    /**
     * Checks if the server is available currently. For more detailed status information
     * see state().
     * @see state()
     */
    static bool isRunning();

    /**
     * Returns the state of the server.
     * @since 4.5
     */
    static State state();

    /**
     * Returns the singleton instance of this class, for connecting to its
     * signals
     */
    static ServerManager* self();

  Q_SIGNALS:
    /**
     * Emitted whenever the server becomes fully operational.
     */
    void started();

    /**
     * Emitted whenever the server becomes unavailable.
     */
    void stopped();

    /**
     * Emitted when the server state changes.
     * @since 4.5
     */
    void stateChanged( Akonadi::ServerManager::State state );

  private:
    //@cond PRIVATE
    friend class ServerManagerPrivate;
    ServerManager( ServerManagerPrivate *dd );
    ServerManagerPrivate* const d;
    Q_PRIVATE_SLOT( d, void serviceOwnerChanged( const QString&, const QString&, const QString& ) )
    Q_PRIVATE_SLOT( d, void checkStatusChanged() )
    Q_PRIVATE_SLOT( d, void timeout() )
    //@endcond
};

}

#endif
