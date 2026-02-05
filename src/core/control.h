/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

#include <QObject>

#include <memory>

namespace Akonadi
{
class ControlPrivate;

/*!
 * \brief Provides methods to control the Akonadi server process.
 *
 * This class provides synchronous methods (ie. use a sub-eventloop)
 * to control the Akonadi service. For asynchronous methods see
 * Akonadi::ServerManager.
 *
 * The most important method in here is widgetNeedsAkonadi(). It is
 * recommended to call it with every top-level widget of your application
 * as argument, assuming your application relies on Akonadi being operational
 * of course.
 *
 * While the Akonadi server automatically started by Akonadi::Session
 * on first use, it might be necessary for some use-cases to guarantee
 * a running Akonadi service at some point. This can be done using
 * start().
 *
 * Example:
 *
 * \code
 *
 * if ( !Akonadi::Control::start() ) {
 *   qDebug() << "Unable to start Akonadi server, exit application";
 *   return 1;
 * } else {
 *   ...
 * }
 *
 * \endcode
 *
 * \class Akonadi::Control
 * \inheaders Akonadi/Control
 * \inmodule AkonadiCore
 *
 * \author Volker Krause <vkrause@kde.org>
 *
 * \sa Akonadi::ServerManager
 */
class AKONADICORE_EXPORT Control : public QObject
{
    Q_OBJECT

public:
    /*!
     * Destroys the control object.
     */
    ~Control() override;

    /*!
     * Starts the Akonadi server synchronously if it is not already running.
     * Returns \\ true if the server was started successfully or was already
     * running, \\ false otherwise
     */
    static bool start();

    /*!
     * Stops the Akonadi server synchronously if it is currently running.
     * Returns \\ true if the server was shutdown successfully or was
     * not running at all, \\ false otherwise.
     * \since 4.2
     */
    static bool stop();

    /*!
     * Restarts the Akonadi server synchronously.
     * Returns \\ true if the restart was successful, \\ false otherwise,
     * the server state is undefined in this case.
     * \since 4.2
     */
    static bool restart();

protected:
    /*!
     * Creates the control object.
     */
    Control();

private:
    std::unique_ptr<ControlPrivate> const d;
};

}
