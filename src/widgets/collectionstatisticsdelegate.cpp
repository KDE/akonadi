/*
    SPDX-FileCopyrightText: 2008 Thomas McGuire <thomas.mcguire@gmx.net>
    SPDX-FileCopyrightText: 2012-2020 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionstatisticsdelegate.h"

#include <KColorScheme>
#include <KFormat>
#include "akonadiwidgets_debug.h"

#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QStyleOptionViewItem>
#include <QAbstractItemView>
#include <QTreeView>

#include "entitytreemodel.h"
#include "collectionstatistics.h"
#include "collection.h"
#include "progressspinnerdelegate_p.h"

using namespace Akonadi;

namespace Akonadi
{

enum CountType {
    UnreadCount,
    TotalCount
};

class CollectionStatisticsDelegatePrivate
{
public:
    QAbstractItemView *parent = nullptr;
    bool drawUnreadAfterFolder = false;
    DelegateAnimator *animator = nullptr;
    QColor mSelectedUnreadColor;
    QColor mDeselectedUnreadColor;

    explicit CollectionStatisticsDelegatePrivate(QAbstractItemView *treeView)
        : parent(treeView)
    {
        updateColor();
    }

    void getCountRecursive(const QModelIndex &index, qint64 &totalCount, qint64 &unreadCount, qint64 &totalSize) const
    {
        Collection collection = qvariant_cast<Collection>(index.data(EntityTreeModel::CollectionRole));
        // Do not assert on invalid collections, since a collection may be deleted
        // in the meantime and deleted collections are invalid.
        if (collection.isValid()) {
            CollectionStatistics statistics = collection.statistics();
            totalCount += qMax(0LL, statistics.count());
            unreadCount += qMax(0LL, statistics.unreadCount());
            totalSize += qMax(0LL, statistics.size());
            if (index.model()->hasChildren(index)) {
                const int rowCount = index.model()->rowCount(index);
                for (int row = 0; row < rowCount; row++) {
                    static const int column = 0;
                    getCountRecursive(index.model()->index(row, column, index), totalCount, unreadCount, totalSize);
                }
            }
        }
    }

    void updateColor()
    {
        mSelectedUnreadColor = KColorScheme(QPalette::Active, KColorScheme::Selection)
                               .foreground(KColorScheme::LinkText).color();
        mDeselectedUnreadColor = KColorScheme(QPalette::Active, KColorScheme::View)
                                 .foreground(KColorScheme::LinkText).color();
    }
};

} // namespace Akonadi

CollectionStatisticsDelegate::CollectionStatisticsDelegate(QAbstractItemView *parent)
    : QStyledItemDelegate(parent)
    , d_ptr(new CollectionStatisticsDelegatePrivate(parent))
{

}

CollectionStatisticsDelegate::CollectionStatisticsDelegate(QTreeView *parent)
    : QStyledItemDelegate(parent)
    , d_ptr(new CollectionStatisticsDelegatePrivate(parent))
{

}

CollectionStatisticsDelegate::~CollectionStatisticsDelegate()
{
    delete d_ptr;
}

void CollectionStatisticsDelegate::setUnreadCountShown(bool enable)
{
    Q_D(CollectionStatisticsDelegate);
    d->drawUnreadAfterFolder = enable;
}

bool CollectionStatisticsDelegate::unreadCountShown() const
{
    Q_D(const CollectionStatisticsDelegate);
    return d->drawUnreadAfterFolder;
}

void CollectionStatisticsDelegate::setProgressAnimationEnabled(bool enable)
{
    Q_D(CollectionStatisticsDelegate);
    if (enable == (d->animator != nullptr)) {
        return;
    }
    if (enable) {
        Q_ASSERT(!d->animator);
        Akonadi::DelegateAnimator *animator = new Akonadi::DelegateAnimator(d->parent);
        d->animator = animator;
    } else {
        delete d->animator;
        d->animator = nullptr;
    }
}

bool CollectionStatisticsDelegate::progressAnimationEnabled() const
{
    Q_D(const CollectionStatisticsDelegate);
    return (d->animator != nullptr);
}

void CollectionStatisticsDelegate::initStyleOption(QStyleOptionViewItem *option,
        const QModelIndex &index) const
{
    Q_D(const CollectionStatisticsDelegate);

    QStyleOptionViewItem *noTextOption =
        qstyleoption_cast<QStyleOptionViewItem *>(option);
    QStyledItemDelegate::initStyleOption(noTextOption, index);
    if (option->decorationPosition != QStyleOptionViewItem::Top) {
        if (noTextOption) {
            noTextOption->text.clear();
        }
    }

    if (d->animator) {

        const QVariant fetchState = index.data(Akonadi::EntityTreeModel::FetchStateRole);
        if (!fetchState.isValid() || fetchState.toInt() != Akonadi::EntityTreeModel::FetchingState) {
            d->animator->pop(index);
            return;
        }

        d->animator->push(index);

        if (QStyleOptionViewItem *v4 = qstyleoption_cast<QStyleOptionViewItem *>(option)) {
            v4->icon = d->animator->sequenceFrame(index);
        }
    }
}

class PainterStateSaver
{
public:
    explicit PainterStateSaver(QPainter *painter)
    {
        mPainter = painter;
        mPainter->save();
    }

    ~PainterStateSaver()
    {
        mPainter->restore();
    }

private:
    Q_DISABLE_COPY(PainterStateSaver)
    QPainter *mPainter = nullptr;
};

void CollectionStatisticsDelegate::paint(QPainter *painter,
        const QStyleOptionViewItem &option,
        const QModelIndex &index) const
{
    Q_D(const CollectionStatisticsDelegate);
    PainterStateSaver stateSaver(painter);

    const QColor textColor = index.data(Qt::ForegroundRole).value<QColor>();
    // First, paint the basic, but without the text. We remove the text
    // in initStyleOption(), which gets called by QStyledItemDelegate::paint().
    QStyledItemDelegate::paint(painter, option, index);

    // Now, we retrieve the correct style option by calling initStyleOption from
    // the superclass.
    QStyleOptionViewItem option4 = option;
    QStyledItemDelegate::initStyleOption(&option4, index);
    QString text = option4.text;

    // Now calculate the rectangle for the text
    QStyle *s = d->parent->style();
    const QWidget *widget = option4.widget;
    const QRect textRect = s->subElementRect(QStyle::SE_ItemViewItemText, &option4, widget);

    // When checking if the item is expanded, we need to check that for the first
    // column, as Qt only recognizes the index as expanded for the first column
    const QModelIndex firstColumn = index.sibling(index.row(), 0);
    QTreeView *treeView = qobject_cast<QTreeView *>(d->parent);
    bool expanded = treeView && treeView->isExpanded(firstColumn);

    if (index.data(EntityTreeModel::PendingCutRole).toBool()) {
        painter->setPen(option.palette.color(QPalette::Disabled, QPalette::Text));
    } else if (option.state & QStyle::State_Selected) {
        painter->setPen(textColor.isValid() ? textColor : option.palette.highlightedText().color());
    } else {
        painter->setPen(textColor.isValid() ? textColor : option.palette.text().color());
    }

    Collection collection = firstColumn.data(EntityTreeModel::CollectionRole).value<Collection>();

    if (!collection.isValid()) {
        qCCritical(AKONADIWIDGETS_LOG) << "Invalid collection at index" << firstColumn << firstColumn.data().toString() << "sibling of" << index << "rowCount=" << index.model()->rowCount(index.parent()) << "parent=" << index.parent().data().toString();
        return;
    }

    CollectionStatistics statistics = collection.statistics();

    qint64 unreadCount = qMax(0LL, statistics.unreadCount());
    qint64 totalRecursiveCount = 0;
    qint64 unreadRecursiveCount = 0;
    qint64 totalSize = 0;
    bool needRecursiveCounts = false;
    bool needTotalSize = false;
    if ((d->drawUnreadAfterFolder && index.column() == 0) || (index.column() == 1 || index.column() == 2)) {
        needRecursiveCounts = true;
    } else if (index.column() == 3 && !expanded) {
        needTotalSize = true;
    }

    if (needRecursiveCounts || needTotalSize) {
        d->getCountRecursive(firstColumn, totalRecursiveCount, unreadRecursiveCount, totalSize);
    }

    // Draw the unread count after the folder name (in parenthesis)
    if (d->drawUnreadAfterFolder && index.column() == 0) {
        // Construct the string which will appear after the foldername (with the
        // unread count)
        QString unread;
//     qCDebug(AKONADIWIDGETS_LOG) << expanded << unreadCount << unreadRecursiveCount;
        if (expanded && unreadCount > 0) {
            unread = QStringLiteral(" (%1)").arg(unreadCount);
        } else if (!expanded) {
            if (unreadCount != unreadRecursiveCount) {
                unread = QStringLiteral(" (%1 + %2)").arg(unreadCount).arg(unreadRecursiveCount - unreadCount);
            } else if (unreadCount > 0) {
                unread = QStringLiteral(" (%1)").arg(unreadCount);
            }
        }

        PainterStateSaver stateSaver(painter);

        if (!unread.isEmpty()) {
            QFont font = painter->font();
            font.setBold(true);
            painter->setFont(font);
        }

        const QColor unreadColor = (option.state & QStyle::State_Selected) ? d->mSelectedUnreadColor : d->mDeselectedUnreadColor;
        const QRect iconRect = s->subElementRect(QStyle::SE_ItemViewItemDecoration, &option4, widget);

        if (option.decorationPosition == QStyleOptionViewItem::Left ||
                option.decorationPosition == QStyleOptionViewItem::Right) {
            // Squeeze the folder text if it is to big and calculate the rectangles
            // where the folder text and the unread count will be drawn to
            QString folderName = text;
            QFontMetrics fm(painter->fontMetrics());
            const int unreadWidth = fm.horizontalAdvance(unread);
            int folderWidth(fm.horizontalAdvance(folderName));
            const bool enoughPlaceForText = (option.rect.width() > (folderWidth + unreadWidth + iconRect.width()));

            if (!enoughPlaceForText && (folderWidth + unreadWidth > textRect.width())) {
                folderName = fm.elidedText(folderName, Qt::ElideRight,
                                           option.rect.width() - unreadWidth - iconRect.width());
                folderWidth = fm.horizontalAdvance(folderName);
            }
            QRect folderRect = textRect;
            QRect unreadRect = textRect;
            folderRect.setRight(textRect.left() + folderWidth);
            unreadRect = QRect(folderRect.right(), folderRect.top(), unreadWidth, unreadRect.height());

            // Draw folder name and unread count
            painter->drawText(folderRect, Qt::AlignLeft | Qt::AlignVCenter, folderName);
            painter->setPen(unreadColor);
            painter->drawText(unreadRect, Qt::AlignLeft | Qt::AlignVCenter, unread);
        } else if (option.decorationPosition == QStyleOptionViewItem::Top) {
            if (unreadCount > 0) {
                // draw over the icon
                // the iconRect is enlarged to the whole width of the item, in case the text is wider than the underlying icon
                painter->setPen(unreadColor);
                painter->drawText(QRect(option.rect.x(), iconRect.y(), option.rect.width(), iconRect.height()), Qt::AlignCenter, QString::number(unreadCount));
            }
        }
        return;
    }

    // For the unread/total column, paint the summed up count if the item
    // is collapsed
    if ((index.column() == 1 || index.column() == 2)) {

        QFont savedFont = painter->font();
        QString sumText;
        if (index.column() == 1 && ((!expanded && unreadRecursiveCount > 0) || (expanded && unreadCount > 0))) {
            QFont font = painter->font();
            font.setBold(true);
            painter->setFont(font);
            sumText = QString::number(expanded ? unreadCount : unreadRecursiveCount);
        } else {

            qint64 totalCount = statistics.count();
            if (index.column() == 2 && ((!expanded && totalRecursiveCount > 0) || (expanded && totalCount > 0))) {
                sumText = QString::number(expanded ? totalCount : totalRecursiveCount);
            }
        }

        painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, sumText);
        painter->setFont(savedFont);
        return;
    }

    //total size
    if (index.column() == 3 && !expanded) {
        painter->drawText(textRect, option4.displayAlignment | Qt::AlignVCenter,
                          KFormat().formatByteSize(totalSize));
        return;
    }

    painter->drawText(textRect, option4.displayAlignment | Qt::AlignVCenter, text);
}

void CollectionStatisticsDelegate::updatePalette()
{
    Q_D(CollectionStatisticsDelegate);
    d->updateColor();
}
