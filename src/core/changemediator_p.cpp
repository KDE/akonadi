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

#include "changemediator_p.h"

#include <QApplication>

#include "changenotificationdependenciesfactory_p.h"
#include "notificationsourceinterface.h"
#include "job_p.h"
#include "itemmovejob.h"
#include "collection.h"
#include "item.h"


//static const char mediatorSessionId[] = "MediatorSession"; TODO: remove?

using namespace Akonadi;

Q_GLOBAL_STATIC(ChangeMediator, s_globalChangeMediator)

ChangeMediator *ChangeMediator::instance()
{
    if (s_globalChangeMediator.isDestroyed()) {
        return nullptr;
    } else {
        return s_globalChangeMediator;
    }
}

ChangeMediator::ChangeMediator(QObject *parent)
    : QObject(parent)
{
    if (qApp) {
        this->moveToThread(qApp->thread());
    }
}

/* static */
void ChangeMediator::registerMonitor(QObject *monitor)
{
    QMetaObject::invokeMethod(instance(), "do_registerMonitor", Q_ARG(QObject *, monitor));
}

/* static */
void ChangeMediator::unregisterMonitor(QObject *monitor)
{
    QMetaObject::invokeMethod(instance(), "do_unregisterMonitor", Q_ARG(QObject *, monitor));
}

/* static */
void ChangeMediator::invalidateCollection(const Akonadi::Collection &collection)
{
    QMetaObject::invokeMethod(instance(), "do_invalidateCollection", Q_ARG(Akonadi::Collection, collection));
}

/* static */
void ChangeMediator::invalidateItem(const Akonadi::Item &item)
{
    QMetaObject::invokeMethod(instance(), "do_invalidateItem", Q_ARG(Akonadi::Item, item));
}

/* static */
void ChangeMediator::invalidateTag(const Tag &tag)
{
    QMetaObject::invokeMethod(instance(), "do_invalidateTag", Q_ARG(Akonadi::Tag, tag));
}

/* static */
void ChangeMediator::updatePendingItem(const Akonadi::Item &item)
{
    QMetaObject::invokeMethod(instance(), "do_updatePendingItem", Q_ARG(Akonadi::Item, item));
}

void ChangeMediator::do_registerMonitor(QObject *monitor)
{
    m_monitors.append(monitor);
}

void ChangeMediator::do_unregisterMonitor(QObject *monitor)
{
    m_monitors.removeAll(monitor);
}

void ChangeMediator::do_invalidateCollection(const Akonadi::Collection &collection)
{
    for (QObject *monitor : qAsConst(m_monitors)) {
        QMetaObject::invokeMethod(monitor, "invalidateCollectionCache", Qt::AutoConnection, Q_ARG(qint64, collection.id()));
    }
}

void ChangeMediator::do_invalidateItem(const Akonadi::Item &item)
{
    for (QObject *monitor : qAsConst(m_monitors)) {
        QMetaObject::invokeMethod(monitor, "invalidateItemCache", Qt::AutoConnection, Q_ARG(qint64, item.id()));
    }
}

void ChangeMediator::do_invalidateTag(const Tag &tag)
{
    for (QObject *monitor : qAsConst(m_monitors)) {
        QMetaObject::invokeMethod(monitor, "invalidateTagCache", Qt::AutoConnection, Q_ARG(qint64, tag.id()));
    }
}

void ChangeMediator::do_updatePendingItem(const Akonadi::Item &item)
{
    for (QObject *monitor : qAsConst(m_monitors)) {
        QMetaObject::invokeMethod(monitor, "updatePendingItem", Qt::AutoConnection, Q_ARG(Akonadi::Item, item));
    }
}
