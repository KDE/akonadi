/*
    Copyright (c) 2010 Michael Jansen <kde@michael-jansen>

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
#ifndef AKONADI_NOTIFICATIONSOURCE_H
#define AKONADI_NOTIFICATIONSOURCE_H

#include "../libs/notificationmessage_p.h"

#include <QtCore/QObject>
#include <QtDBus/QtDBus>


namespace Akonadi {

class NotificationManager;

/**
 * @brief "short description"
 *
 * @since :RELEASE_VERSION:
 */
class NotificationSource : public QObject
    {
    Q_OBJECT
    Q_CLASSINFO( "D-Bus Interface", "org.freedesktop.Akonadi.NotificationSource" )

  public:

    /**
     * Construct a NotificationSource.
     *
     * @param parent The parent object.
     */
    NotificationSource( const QString &identifier, Akonadi::NotificationManager *parent );

    /**
     * Destroy the NotificationSource.
     */
    virtual ~NotificationSource();

    /**
     * Emit the given notifications
     *
     * @param notifications List of notifications to emit.
     */
    void emitNotification( const NotificationMessage::List &notifications );

    /**
     * Return the dbus path this message source uses.
     */
    QDBusObjectPath dbusPath() const;

    /**
     * Return the identifier for this message source
     */
    QString identifier() const;

    public Q_SLOTS:

      /**
       * Unsubscribe from the message source.
       *
       * This will delete the message source and make the used dbus path unavailable.
       */
      Q_SCRIPTABLE void unsubscribe();

    Q_SIGNALS:

      Q_SCRIPTABLE void notify( const Akonadi::NotificationMessage::List &msgs );

    private:

      Akonadi::NotificationManager *mManager;
      QString mIdentifier;
      QString mDBusIdentifier;

    }; // class NotificationSource




} // namespace Akonadi



#endif // #define AKONADI_NOTIFICATIONSOURCE_H

