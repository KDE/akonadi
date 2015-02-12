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

#ifndef PROGRESSSPINNERDELEGATE_P_H
#define PROGRESSSPINNERDELEGATE_P_H

#include <QStyledItemDelegate>
#include <QSet>

#include <kpixmapsequence.h>

namespace Akonadi {

class DelegateAnimator : public QObject
{
    Q_OBJECT
public:
    explicit DelegateAnimator(QAbstractItemView *view);

    void push(const QModelIndex &index);
    void pop(const QModelIndex &index);

    QPixmap sequenceFrame(const QModelIndex &index);

    static const int sCount = 7;
    struct Animation {
        inline Animation(const QPersistentModelIndex &idx)
            : frame(0)
            , index(idx)
        {
        }

        bool operator==(const Animation &other) const
        {
            return index == other.index;
        }

        inline void nextFrame() const {
            frame = (frame + 1) % sCount;
        }
        mutable int frame;
        QPersistentModelIndex index;
    };

protected:
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;

private:

    QSet<Animation> m_animations;
    QAbstractItemView *m_view;
    KPixmapSequence m_pixmapSequence;
    int m_timerId;
};

uint qHash(Akonadi::DelegateAnimator::Animation anim);

/**
 *
 */
class ProgressSpinnerDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ProgressSpinnerDelegate(DelegateAnimator *animator, QObject *parent = 0);

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const Q_DECL_OVERRIDE;

private:
    DelegateAnimator *m_animator;
};

}

#endif
