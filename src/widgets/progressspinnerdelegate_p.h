/*
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net
    SPDX-FileContributor: Stephen Kelly <stephen@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QSet>
#include <QStyledItemDelegate>

#include <KPixmapSequence>

namespace Akonadi
{
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

        inline void nextFrame() const
        {
            frame = (frame + 1) % sCount;
        }
        mutable int frame;
        QPersistentModelIndex index;
    };

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    QSet<Animation> m_animations;
    QAbstractItemView *const m_view;
    KPixmapSequence m_pixmapSequence;
    int m_timerId = -1;
};

uint qHash(const Akonadi::DelegateAnimator::Animation &anim);

/**
 *
 */
class ProgressSpinnerDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ProgressSpinnerDelegate(DelegateAnimator *animator, QObject *parent = nullptr);

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;

private:
    DelegateAnimator *const m_animator;
};

}
