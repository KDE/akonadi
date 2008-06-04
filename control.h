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
 * This class provides a method to start the Akonadi server
 * process synchronously.
 *
 * Normally the Akonadi server is started by the KDE session
 * manager, however for unit tests or special needs one can
 * use this class to start it explicitly.
 *
 * Example:
 *
 * @code
 *
 * if ( !Akonadi::Control::start() ) {
 *   qDebug() << "Unable to start server, exit application";
 *   return 1;
 * } else {
 *   ...
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
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
     */
    static bool start();

  protected:
    /**
     * Creates the control object.
     */
    Control();

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void serviceOwnerChanged( const QString&, const QString&, const QString& ) )
    //@endcond
};

}

#endif
