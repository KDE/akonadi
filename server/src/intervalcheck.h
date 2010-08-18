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

#ifndef INTERVALCHECK_H
#define INTERVALCHECK_H

#include "entities.h"

#include <QDateTime>
#include <QHash>
#include <QThread>
#include <QMutex>

using namespace Akonadi;

/**
  Interval checking thread.
*/
class IntervalCheck : public QThread
{
  Q_OBJECT

  public:
    IntervalCheck( QObject *parent = 0 );
    ~IntervalCheck();

    static IntervalCheck* instance();

    /**
     * Requests the given collection to be synced.
     * Executed from any thread, forwards to triggerCollectionXSync() in the
     * retrieval thread.
     * A minimum time interval between two sync requests is ensured.
     */
    void requestCollectionSync( const Collection &collection );

  protected:
    void run();

  private slots:
    void doIntervalCheck();

  private:
    static IntervalCheck* s_instance;
    QMutex m_lastSyncMutex;
    QHash<int, QDateTime> mLastChecks;
    QHash<QString, QDateTime> mLastCollectionTreeSyncs;
};

#endif
