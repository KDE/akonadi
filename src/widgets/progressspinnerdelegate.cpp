/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Stephen Kelly <stephen@kdab.com>

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

#include "progressspinnerdelegate_p.h"


#include "entitytreemodel.h"

#include <KIconLoader>

#include <QTimerEvent>
#include <QAbstractItemView>

using namespace Akonadi;

DelegateAnimator::DelegateAnimator(QAbstractItemView *view)
    : QObject(view)
    , m_view(view)
    , m_timerId(-1)
{
    m_pixmapSequence = KIconLoader::global()->loadPixmapSequence(QStringLiteral("process-working"), 22);
}

void DelegateAnimator::push(const QModelIndex &index)
{
    if (m_animations.isEmpty()) {
        m_timerId = startTimer(200);
    }
    m_animations.insert(Animation(index));
}

void DelegateAnimator::pop(const QModelIndex &index)
{
    if (m_animations.remove(Animation(index))) {
        if (m_animations.isEmpty() && m_timerId != -1) {
            killTimer(m_timerId);
            m_timerId = -1;
        }
    }
}

void DelegateAnimator::timerEvent(QTimerEvent *event)
{
    if (!(event->timerId() == m_timerId && m_view)) {
        return QObject::timerEvent(event);
    }

    QRegion region;
    // Do no port this to for(:)! The pop() inside the loop invalidates (even implicit) iterators.
    Q_FOREACH (const Animation &animation, m_animations) {
        // Check if loading is finished (we might not be notified, if the index is scrolled out of view)
        const QVariant fetchState = animation.index.data(Akonadi::EntityTreeModel::FetchStateRole);
        if (fetchState.toInt() != Akonadi::EntityTreeModel::FetchingState) {
            pop(animation.index);
            continue;
        }

        // This repaints the entire delegate (icon and text).
        // TODO: See if there's a way to repaint only part of it (the icon).
        animation.nextFrame();
        const QRect rect = m_view->visualRect(animation.index);
        region += rect;
    }

    if (!region.isEmpty()) {
        m_view->viewport()->update(region);
    }
}

QPixmap DelegateAnimator::sequenceFrame(const QModelIndex &index)
{
    for (const Animation &animation : qAsConst(m_animations)) {
        if (animation.index == index) {
            return m_pixmapSequence.frameAt(animation.frame);
        }
    }
    return QPixmap();
}

ProgressSpinnerDelegate::ProgressSpinnerDelegate(DelegateAnimator *animator, QObject *parent)
    : QStyledItemDelegate(parent)
    , m_animator(animator)
{

}

void ProgressSpinnerDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);

    const QVariant fetchState = index.data(Akonadi::EntityTreeModel::FetchStateRole);
    if (!fetchState.isValid() || fetchState.toInt() != Akonadi::EntityTreeModel::FetchingState) {
        m_animator->pop(index);
        return;
    }

    m_animator->push(index);

    if (QStyleOptionViewItem *v = qstyleoption_cast<QStyleOptionViewItem *>(option)) {
        v->icon = m_animator->sequenceFrame(index);
    }
}

uint Akonadi::qHash(const Akonadi::DelegateAnimator::Animation &anim)
{
    return qHash(anim.index);
}
