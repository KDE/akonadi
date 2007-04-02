/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef DBUSTHREAD_H
#define DBUSTHREAD_H

#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QThread>
#include <QtCore/QVariant>
#include <QtCore/QWaitCondition>
#include <QtDBus/QDBusMessage>
#include "akonadiprivate_export.h"

/**
 * Threads which want to send DBus messages must inherit from
 * this class.
 *
 * To send a message use callDBus.
 */
class AKONADIPRIVATE_EXPORT DBusThread : public QThread
{
  friend class DBusThreadProxy;

  public:
    DBusThread( QObject *parent = 0 );

    QList<QVariant> callDBus( const QString &service, const QString &path,
                              const QString &inteface, const QString &method,
                              const QList<QVariant> &arguments );

  private:
    void wakeup( const QList<QVariant> &results );

    QList<QVariant> mData;
    QMutex mMutex;
    QWaitCondition mCondition;
};

/**
 * Internal class.
 */
class DBusThreadProxy : public QObject
{
  Q_OBJECT

  public:
    DBusThreadProxy( DBusThread *thread, const QDBusMessage &message );

  private Q_SLOTS:
    void dbusReply( const QDBusMessage& );
    void dbusError( const QDBusError &err, const QDBusMessage &msg );

  private:
    QPointer<DBusThread> mThread;
};

#endif
