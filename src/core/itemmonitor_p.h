/*
    SPDX-FileCopyrightText: 2007-2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_ITEMMONITOR_P_H
#define AKONADI_ITEMMONITOR_P_H

#include <QObject>

#include "monitor.h"

namespace Akonadi
{

/**
 * @internal
 */
class Q_DECL_HIDDEN ItemMonitor::Private : public QObject
{
    Q_OBJECT

public:
    Private(ItemMonitor *parent)
        : QObject(nullptr)
        , mParent(parent)
        , mMonitor(new Monitor())
    {
        mMonitor->setObjectName(QStringLiteral("ItemMonitorMonitor"));
        connect(mMonitor, &Monitor::itemChanged,
                this, &Private::slotItemChanged);
        connect(mMonitor, &Monitor::itemRemoved,
                this, &Private::slotItemRemoved);
    }

    ~Private()
    {
        delete mMonitor;
    }

    ItemMonitor *mParent = nullptr;
    Item mItem;
    Monitor *mMonitor = nullptr;

private Q_SLOTS:
    void slotItemChanged(const Akonadi::Item &item, const QSet<QByteArray> &aSet)
    {
        Q_UNUSED(aSet)
        mItem.apply(item);
        mParent->itemChanged(item);
    }

    void slotItemRemoved(const Akonadi::Item &item)
    {
        Q_UNUSED(item)
        mItem = Item();
        mParent->itemRemoved();
    }
};

} // namespace Akonadi

#endif
