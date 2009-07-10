/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_CONTROL_H
#define AKONADI_CONTROL_H

#include "akonadi_export.h"
#include <QtCore/QObject>


namespace Akonadi {

/**
 * @short Provides methods to control the Akonadi server process.
 *
 * This class provides high-level methods to control the Akonadi
 * server. These methods are synchronously (ie. use a sub-eventloop)
 * and can show dialogs. For more low-level methods see
 * Akonadi::ServerManager.
 *
 * While the Akonadi server normally is started by the KDE session
 * manager, it is not guaranteed that your application is running
 * inside a KDE session. Therefore it is recommended to execute
 * Akonadi::Control::start() during startup to ensure the Akonadi
 * server is running.
 *
 * Example:
 *
 * @code
 *
 * if ( !Akonadi::Control::start() ) {
 *   qDebug() << "Unable to start Akonadi server, exit application";
 *   return 1;
 * } else {
 *   ...
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 *
 * @see Akonadi::ServerManager
 */
class AKONADI_EXPORT Control : public QObject
{
  Q_OBJECT

  public:
    /**
     * Destroys the control object.
     */
    ~Control();

    /**
     * Starts the Akonadi server synchronously if it is not already running.
     * @return @c true if the server was started successfully or was already
     * running, @c false otherwise
     */
    static bool start();

    /**
     * Same as start(), but with GUI feedback.
     * @param parent The parent widget.
     * @since 4.2
     */
    static bool start( QWidget *parent );

    /**
     * Stops the Akonadi server synchronously if it is currently running.
     * @return @c true if the server was shutdown successfully or was
     * not running at all, @c false otherwise.
     * @since 4.2
     */
    static bool stop();

    /**
     * Same as stop(), but with GUI feedback.
     * @param parent The parent widget.
     * @since 4.2
     */
    static bool stop( QWidget *parent );

    /**
     * Restarts the Akonadi server synchronously.
     * @return @c true if the restart was successful, @c false otherwise,
     * the server state is undefined in this case.
     * @since 4.2
     */
    static bool restart();

    /**
     * Same as restart(), but with GUI feedback.
     * @param parent The parent widget.
     * @since 4.2
     */
    static bool restart( QWidget *parent );

    /**
     * Disable the given widget when Akonadi is not operational and show
     * an error overlay (given enough space). Cascading use is automatically
     * detected.
     * @param widget The widget depending on Akonadi being operational.
     * @since 4.2
     */
    static void widgetNeedsAkonadi( QWidget *widget );

  protected:
    /**
     * Creates the control object.
     */
    Control();

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void serverStarted() )
    Q_PRIVATE_SLOT( d, void serverStopped() )
    Q_PRIVATE_SLOT( d, void createErrorOverlays() )
    Q_PRIVATE_SLOT( d, void cleanup() )
    //@endcond
};

}

#endif
