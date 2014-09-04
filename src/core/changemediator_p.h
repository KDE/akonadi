/*
    Copyright (c) 2011 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2011 Stephen Kelly <steveire@gmail.com>

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

#ifndef AKONADI_CHANGEMEDIATOR_P_H
#define AKONADI_CHANGEMEDIATOR_P_H

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QSet>

#include "item.h"
#include <akonadi/private/notificationmessagev2_p.h>

namespace Akonadi {

class Job;
class JobPrivate;

class Collection;
class Item;

class ChangeMediator : public QObject
{
    Q_OBJECT
public:
    explicit ChangeMediator(QObject *parent = 0);

    static ChangeMediator *instance();

    static void registerMonitor(QObject *monitor);
    static void unregisterMonitor(QObject *monitor);

    static void invalidateCollection(const Akonadi::Collection &collection);
    static void invalidateItem(const Akonadi::Item &item);
    static void invalidateTag(const Akonadi::Tag &tag);

    static void registerSession(const QByteArray &id);
    static void unregisterSession(const QByteArray &id);
    static void beginMoveItems(JobPrivate *movePrivate, const QByteArray &id);
    static void itemsMoved(const Item::List &items, const Collection &sourceParent, const QByteArray &id);

private Q_SLOTS:
    void do_registerMonitor(QObject *monitor);
    void do_unregisterMonitor(QObject *monitor);

    void do_invalidateCollection(const Akonadi::Collection &collection);
    void do_invalidateItem(const Akonadi::Item &item);
    void do_invalidateTag(const Akonadi::Tag &tag);

    void do_registerSession(const QByteArray &id);
    void do_unregisterSession(const QByteArray &id);
    void do_beginMoveItems(JobPrivate *movePrivate, const QByteArray &id);
    void do_itemsMoved(const Item::List &items, const Collection &sourceParent, const QByteArray &id);

private:
    QList<QObject *> m_monitors;

    QVector<Akonadi::NotificationMessageV2> messageQueue;
    QVector<Akonadi::Job *> unfilteredJobs;

    QSet<QByteArray> m_sessions;
};

}

#endif
