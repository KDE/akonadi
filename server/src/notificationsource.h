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
#include "../libs/notificationmessagev2_p.h"

#include <QtCore/QObject>
#include <QtDBus/QtDBus>

#include "entities.h"

namespace Akonadi {

class NotificationManager;

class NotificationSource : public QObject
{
    Q_OBJECT
    Q_CLASSINFO( "D-Bus Interface", "org.freedesktop.Akonadi.NotificationSource" )

  public:

    /**
     * Construct a NotificationSource.
     *
     * @param identifier The identifier of this notification source, defined by the client
     * @param clientServiceName The D-Bus service name of the client, used to clean up if the client does not unsubscribe correctly.
     * @param parent The parent object.
     */
    NotificationSource( const QString &identifier, const QString &clientServiceName, Akonadi::NotificationManager *parent );

    /**
     * Destroy the NotificationSource.
     */
    virtual ~NotificationSource();

    /**
     * Emit the given notifications
     *
     * @param notifications List of notifications to emit.
     */
    void emitNotification( const NotificationMessageV2::List &notifications );

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

    /**
     * Add another client service to watch for. Auto-unsubscription only happens if
     * all watched client services have been stopped.
     */
    void addClientServiceName( const QString &clientServiceName );

  public Q_SLOTS:

    /**
      * Unsubscribe from the message source.
      *
      * This will delete the message source and make the used dbus path unavailable.
      */
    Q_SCRIPTABLE void unsubscribe();

    Q_SCRIPTABLE void setMonitoredCollection( Entity::Id id, bool monitored );
    Q_SCRIPTABLE QVector<Entity::Id> monitoredCollections() const;
    Q_SCRIPTABLE void setMonitoredItem( Entity::Id id, bool monitored );
    Q_SCRIPTABLE QVector<Entity::Id> monitoredItems() const;
    Q_SCRIPTABLE void setMonitoredResource( const QByteArray &resource, bool monitored );
    Q_SCRIPTABLE QVector<QByteArray> monitoredResources() const;
    Q_SCRIPTABLE void setMonitoredMimeType( const QString &mimeType, bool monitored );
    Q_SCRIPTABLE QStringList monitoredMimeTypes() const;
    Q_SCRIPTABLE void setAllMonitored( bool allMonitored );
    Q_SCRIPTABLE bool isAllMonitored() const;
    Q_SCRIPTABLE void setIgnoredSession( const QByteArray &sessionId, bool ignored );
    Q_SCRIPTABLE QVector<QByteArray> ignoredSessions() const;

  Q_SIGNALS:

    Q_SCRIPTABLE void notify( const Akonadi::NotificationMessage::List &msgs );
    Q_SCRIPTABLE void notifyV2( const Akonadi::NotificationMessageV2::List &msgs );

    Q_SCRIPTABLE void monitoredCollectionsChanged();
    Q_SCRIPTABLE void monitoredItemsChanged();
    Q_SCRIPTABLE void monitoredResourcesChanged();
    Q_SCRIPTABLE void monitoredMimeTypesChanged();
    Q_SCRIPTABLE void isAllMonitoredChanged();
    Q_SCRIPTABLE void ignoredSessionsChanged();

  private Q_SLOTS:
    void serviceUnregistered( const QString &serviceName );

  private:
    bool isServerSideMonitorEnabled() const;

  private:
    Akonadi::NotificationManager *mManager;
    QString mIdentifier;
    QString mDBusIdentifier;
    QDBusServiceWatcher* mClientWatcher;

}; // class NotificationSource




} // namespace Akonadi



#endif // #define AKONADI_NOTIFICATIONSOURCE_H

