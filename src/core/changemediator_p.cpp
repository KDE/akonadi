/*
    SPDX-FileCopyrightText: 2011 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2011 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
    if (auto app = QCoreApplication::instance(); app != nullptr) {
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
        for (auto monitor : qAsConst(instance()->m_monitors)) {
            const bool ok = QMetaObject::invokeMethod(monitor, "invalidateCollectionCache", Q_ARG(qint64, colId));
            Q_ASSERT(ok);
            Q_UNUSED(ok)
        }
    });
}

/* static */
void ChangeMediator::invalidateItem(const Akonadi::Item &item)
{
    QMetaObject::invokeMethod(instance(), [itemId = item.id()]() {
        for (auto monitor : qAsConst(instance()->m_monitors)) {
            const bool ok = QMetaObject::invokeMethod(monitor, "invalidateItemCache", Q_ARG(qint64, itemId));
            Q_ASSERT(ok);
            Q_UNUSED(ok)
        }
    });
}

/* static */
void ChangeMediator::invalidateTag(const Tag &tag)
{
    QMetaObject::invokeMethod(instance(), [tagId = tag.id()]() {
        for (auto monitor : qAsConst(instance()->m_monitors)) {
            const bool ok = QMetaObject::invokeMethod(monitor, "invalidateTagCache", Q_ARG(qint64, tagId));
            Q_ASSERT(ok);
            Q_UNUSED(ok)
        }
    });
}

#include "changemediator_p.moc"
