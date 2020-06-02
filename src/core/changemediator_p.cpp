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

#include <QCoreApplication>

#include "collection.h"
#include "item.h"

using namespace Akonadi;

class GlobalChangeMediator : public ChangeMediator
{
    Q_OBJECT
};

Q_GLOBAL_STATIC(GlobalChangeMediator, s_globalChangeMediator) // NOLINT(readability-redundant-member-init)

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
    if (auto *app = QCoreApplication::instance(); app != nullptr) {
        this->moveToThread(app->thread());
    }
}

/* static */
void ChangeMediator::registerMonitor(QObject *monitor)
{
    QMetaObject::invokeMethod(instance(), [monitor]() {
        instance()->m_monitors.push_back(monitor);
    });
}

/* static */
void ChangeMediator::unregisterMonitor(QObject *monitor)
{
    QMetaObject::invokeMethod(instance(), [monitor]() {
        instance()->m_monitors.removeAll(monitor);
    });
}

/* static */
void ChangeMediator::invalidateCollection(const Akonadi::Collection &collection)
{
    QMetaObject::invokeMethod(instance(), [colId = collection.id()]() {
        for (auto *monitor : qAsConst(instance()->m_monitors)) {
            const bool ok = QMetaObject::invokeMethod(monitor, "invalidateCollectionCache", Q_ARG(qint64, colId));
            Q_ASSERT(ok); Q_UNUSED(ok);
        }
    });
}

/* static */
void ChangeMediator::invalidateItem(const Akonadi::Item &item)
{
    QMetaObject::invokeMethod(instance(), [itemId = item.id()]() {
        for (auto *monitor : qAsConst(instance()->m_monitors)) {
            const bool ok = QMetaObject::invokeMethod(monitor, "invalidateItemCache", Q_ARG(qint64, itemId));
            Q_ASSERT(ok); Q_UNUSED(ok);
        }
    });
}

/* static */
void ChangeMediator::invalidateTag(const Tag &tag)
{
    QMetaObject::invokeMethod(instance(), [tagId = tag.id()]() {
        for (auto *monitor : qAsConst(instance()->m_monitors)) {
            const bool ok = QMetaObject::invokeMethod(monitor, "invalidateTagCache", Q_ARG(qint64, tagId));
            Q_ASSERT(ok); Q_UNUSED(ok);
        }
    });
}

#include "changemediator_p.moc"
