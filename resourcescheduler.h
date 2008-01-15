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

#ifndef AKONADI_RESOURCESCHEDULER_H
#define AKONADI_RESOURCESCHEDULER_H

#include <libakonadi/collection.h>
#include <libakonadi/item.h>

#include <QtCore/QObject>
#include <QDBusMessage>

namespace Akonadi {

/**
  @internal

  Manages synchronization and fetch requests for a resource.

  @todo Attach to the ResourceBase Monitor,
*/
class ResourceScheduler : public QObject
{
  Q_OBJECT

  public:
    enum TaskType {
      Invalid,
      SyncAll,
      SyncCollectionTree,
      SyncCollection,
      FetchItem,
      ChangeReplay
    };

    class Task {
      public:
        Task() : type( Invalid ) {}
        TaskType type;
        Collection collection;
        Item item;
        QStringList itemParts;
        QDBusMessage dbusMsg;

        bool operator==( const Task &other ) const
        {
          return type == other.type
              && collection == other.collection
              && item.reference() == other.item.reference()
              && itemParts == other.itemParts;
        }
    };

    ResourceScheduler( QObject *parent = 0 );

    /**
      Schedules a full synchronization.
    */
    void scheduleFullSync();

    /**
      Schedules a collection tree sync.
    */
    void scheduleCollectionTreeSync();

    /**
      Schedules the synchronization of a single collection.
      @param col The collection to synchronize.
    */
    void scheduleSync( const Collection &col, const QStringList &parts );

    /**
      Schedules fetching of a single PIM item.
      @param item the item to fetch.
      @param parts List of names of the parts of the item to fetch.
      @param msg The associated D-Bus message.
    */
    void scheduleItemFetch( const Item &item, const QStringList &parts, const QDBusMessage &msg );

    /**
      The current task has been finished
    */
    void taskDone();

    /**
      Returns the current task.
    */
    Task currentTask() const;

    /**
      Sets the online state.
    */
    void setOnline( bool state );

  public Q_SLOTS:
    /**
      Schedules replaying changes.
    */
    void scheduleChangeReplay();

  Q_SIGNALS:
    void executeFullSync();
    void executeCollectionSync( const Akonadi::Collection &col, const QStringList &parts );
    void executeCollectionTreeSync();
    void executeItemFetch( const Akonadi::Item &item, const QStringList &parts );
    void executeChangeReplay();

  private slots:
    void scheduleNext();
    void executeNext();

  private:
    QList<Task> mTaskList;
    Task mCurrentTask;
    bool mOnline;
};

}

#endif
