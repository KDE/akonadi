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
#include <libakonadi/datareference.h>

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
        DataReference itemRef;
        QStringList itemParts;
        QDBusMessage dbusMsg;
    };

    ResourceScheduler( QObject *parent = 0 );

    /**
      Schedules a full synchronization.
    */
    void scheduleFullSync();

    /**
      Schedules the synchronization of a single collection.
      @param col The collection to synchronize.
    */
    void scheduleSync( const Collection &col );

    /**
      Schedules fetching of a single PIM item.
      @param ref DataReference to the item to fetch.
      @param parts List of names of the parts of the item to fetch.
      @param msg The associated DBus message.
    */
    void scheduleItemFetch( const DataReference &ref, const QStringList &parts, const QDBusMessage &msg );

    /**
      The current task has been finished
    */
    void taskDone();

    /**
      Returns the current task.
    */
    Task currentTask() const;

  public Q_SLOTS:
    /**
      Schedules replaying changes.
    */
    void scheduleChangeReplay();

  Q_SIGNALS:
    void executeFullSync();
    void executeCollectionSync( const Collection &col );
    void executeItemFetch( const DataReference &ref, const QStringList &parts, const QDBusMessage &msg );
    void executeChangeReplay();

  private slots:
    void scheduleNext();
    void executeNext();

  private:
    QList<Task> mTaskList;
    Task mCurrentTask;
};

}

#endif
