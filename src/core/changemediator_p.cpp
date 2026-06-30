/*
    SPDX-FileCopyrightText: 2011 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2011 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "changemediator_p.h"

#include <QCoreApplication>

#include "collection.h"
#include "item.h"
#include "monitor.h"
#include "monitor_p.h"

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
    if (auto app = QCoreApplication::instance(); app != nullptr) {
        this->moveToThread(app->thread());
    }
}

/* static */
void ChangeMediator::registerMonitor(const Monitor *monitor)
{
    QMetaObject::invokeMethod(instance(), [monitor]() {
        instance()->m_monitors.push_back(monitor);
    });
}

/* static */
void ChangeMediator::unregisterMonitor(const Monitor *monitor)
{
    QMetaObject::invokeMethod(instance(), [monitor]() {
        instance()->m_monitors.removeAll(monitor);
    });
}

/* static */
void ChangeMediator::invalidateCollection(const Akonadi::Collection &collection)
{
    QMetaObject::invokeMethod(instance(), [colId = collection.id()]() {
        for (auto monitor : std::as_const(instance()->m_monitors)) {
            monitor->d_ptr->invalidateCollectionCache(colId);
        }
    });
}

/* static */
void ChangeMediator::invalidateItem(const Akonadi::Item &item)
{
    QMetaObject::invokeMethod(instance(), [itemId = item.id()]() {
        for (auto monitor : std::as_const(instance()->m_monitors)) {
            monitor->d_ptr->invalidateItemCache(itemId);
        }
    });
}

/* static */
void ChangeMediator::invalidateTag(const Tag &tag)
{
    QMetaObject::invokeMethod(instance(), [tagId = tag.id()]() {
        for (auto monitor : std::as_const(instance()->m_monitors)) {
            monitor->d_ptr->invalidateTagCache(tagId);
        }
    });
}

#include "changemediator_p.moc"

#include "moc_changemediator_p.cpp"
