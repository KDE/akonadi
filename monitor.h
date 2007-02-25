/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_MONITOR_H
#define AKONADI_MONITOR_H

#include <libakonadi/job.h>
#include <QtCore/QObject>

namespace Akonadi {

class MonitorPrivate;
class Session;

/**
  Monitors an Item or Collection for changes and emits signals if some
  of these objects are changed or removed or new ones are added to the storage
  backend.

  @todo: support un-monitoring
  @todo: distinguish between monitoring collection properties and collection content.
  @todo: special case for collection content counts changed
*/
class AKONADI_EXPORT Monitor : public QObject
{
  Q_OBJECT

  public:
    /**
      Creates a new monitor.
      @param parent The parent object.
    */
    Monitor( QObject *parent = 0 );

    /**
      Destroys this monitor.
    */
    virtual ~Monitor();

    /**
      Monitors the specified collection for changes.
      @param path The collection path.
      @param recursive If true also sub-collection will be monitored.
    */
    void monitorCollection( const QString &path, bool recursive );

    /**
      Monitors the specified PIM Item for changes.
      @param ref The item references.
    */
    void monitorItem( const DataReference &ref );

    /**
      Monitors the specified resource for changes.
      @param resource The resource identifier.
    */
    void monitorResource( const QByteArray &resource );

    /**
      Monitor all items matching the specified mimetype.
      @param mimetype The mimetype.
    */
    void monitorMimeType( const QByteArray &mimetype );

    /**
      Monitor all items.
    */
    void monitorAll();

    /**
      Ignore all notifications caused by the given session.
      @param session The session you want to ignore.
    */
    void ignoreSession( Session *session );

  Q_SIGNALS:
    /**
      Emitted if a monitored object has changed.
      @param ref Reference of the changed objects.
    */
    void itemChanged( const Akonadi::DataReference &ref );

    /**
      Emitted if a item has been added to a monitored collection.
      @param ref Reference of the added object.
    */
    void itemAdded( const Akonadi::DataReference &ref );

    /**
      Emitted if a monitored object has been removed.
      @param ref Reference of the removed object.
    */
    void itemRemoved( const Akonadi::DataReference &ref);

    /**
      Emitted if a monitored got a new child collection.
      @param path The path of the new collection.
    */
    void collectionAdded( const QString &path );

    /**
      Emitted if a monitored collection changed (its properties, not its
      content).
      @param path The path of the modified collection.
    */
    void collectionChanged( const QString &path );

    /**
      Emitted if a monitored collection has been removed.
      @param path The path of the rmeoved collection.
    */
    void collectionRemoved( const QString &path );

  private:
    bool connectToNotificationManager();

  private Q_SLOTS:
    void slotItemChanged( const QByteArray &sessionId, int uid, const QString &remoteId, const QString &collection,
                          const QByteArray &mimetype, const QByteArray &resource );
    void slotItemAdded( const QByteArray &sessionId, int uid, const QString &remoteId, const QString &collection,
                        const QByteArray &mimetype, const QByteArray &resource );
    void slotItemRemoved( const QByteArray &sessionId, int uid, const QString &remoteId, const QString &collection,
                          const QByteArray &mimetype, const QByteArray &resource );
    void slotCollectionAdded( const QByteArray &sessionId, const QString &path, const QByteArray &resource );
    void slotCollectionChanged( const QByteArray &sessionId, const QString &path, const QByteArray &resource );
    void slotCollectionRemoved( const QByteArray &sessionId, const QString &path, const QByteArray &resource );

    void sessionDestroyed(QObject *obj);

  private:
    MonitorPrivate* const d;
};

}

#endif
