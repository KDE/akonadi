/*
    SPDX-FileCopyrightText: 2006-2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agenttypewidget.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QListView>
#include <QPainter>

#include "agentfilterproxymodel.h"
#include "agenttype.h"
#include "agenttypemodel.h"

namespace Akonadi
{
namespace Internal
{
/**
 * @internal
 */
class AgentTypeWidgetDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    explicit AgentTypeWidgetDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    void drawFocus(QPainter * /*painter*/, const QStyleOptionViewItem & /*option*/, QRect /*rect*/) const;
};

} // namespace Internal

using Akonadi::Internal::AgentTypeWidgetDelegate;

/**
 * @internal
 */
class AgentTypeWidgetPrivate
{
public:
    explicit AgentTypeWidgetPrivate(AgentTypeWidget *parent)
        : mParent(parent)
    {
    }

    void currentAgentTypeChanged(const QModelIndex & /*currentIndex*/, const QModelIndex & /*previousIndex*/);

    void typeActivated(const QModelIndex &index)
    {
        if (index.flags() & (Qt::ItemIsSelectable | Qt::ItemIsEnabled)) {
            Q_EMIT mParent->activated();
        }
    }

    AgentTypeWidget *const mParent;
    QListView *mView = nullptr;
    AgentTypeModel *mModel = nullptr;
    AgentFilterProxyModel *proxyModel = nullptr;
};

void AgentTypeWidgetPrivate::currentAgentTypeChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex)
{
    AgentType currentType;
    if (currentIndex.isValid()) {
        currentType = currentIndex.data(AgentTypeModel::TypeRole).value<AgentType>();
    }

    AgentType previousType;
    if (previousIndex.isValid()) {
        previousType = previousIndex.data(AgentTypeModel::TypeRole).value<AgentType>();
    }

    Q_EMIT mParent->currentChanged(currentType, previousType);
}

AgentTypeWidget::AgentTypeWidget(QWidget *parent)
    : QWidget(parent)
    , d(new AgentTypeWidgetPrivate(this))
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins({});

    d->mView = new QListView(this);
    d->mView->setItemDelegate(new AgentTypeWidgetDelegate(d->mView));
    d->mView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    d->mView->setAlternatingRowColors(true);
    layout->addWidget(d->mView);

    d->mModel = new AgentTypeModel(d->mView);
    d->proxyModel = new AgentFilterProxyModel(this);
    d->proxyModel->setSourceModel(d->mModel);
    d->proxyModel->sort(0);
    d->mView->setModel(d->proxyModel);

    d->mView->selectionModel()->setCurrentIndex(d->mView->model()->index(0, 0), QItemSelectionModel::Select);
    d->mView->scrollTo(d->mView->model()->index(0, 0));
    connect(d->mView->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &start, const QModelIndex &end) {
        d->currentAgentTypeChanged(start, end);
    });
    connect(d->mView, qOverload<const QModelIndex &>(&QListView::activated), this, [this](const QModelIndex &index) {
        d->typeActivated(index);
    });
}

AgentTypeWidget::~AgentTypeWidget() = default;

AgentType AgentTypeWidget::currentAgentType() const
{
    QItemSelectionModel *selectionModel = d->mView->selectionModel();
    if (!selectionModel) {
        return AgentType();
    }

    QModelIndex index = selectionModel->currentIndex();
    if (!index.isValid()) {
        return AgentType();
    }

    return index.data(AgentTypeModel::TypeRole).value<AgentType>();
}

AgentFilterProxyModel *AgentTypeWidget::agentFilterProxyModel() const
{
    return d->proxyModel;
}

/**
 * AgentTypeWidgetDelegate
 */

AgentTypeWidgetDelegate::AgentTypeWidgetDelegate(QObject *parent)
    : QAbstractItemDelegate(parent)
{
}

void AgentTypeWidgetDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid()) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing);

    const QString name = index.model()->data(index, Qt::DisplayRole).toString();
    const QString comment = index.model()->data(index, AgentTypeModel::DescriptionRole).toString();

    const QVariant data = index.model()->data(index, Qt::DecorationRole);

    QPixmap pixmap;
    if (data.isValid() && data.typeId() == QMetaType::QIcon) {
        pixmap = qvariant_cast<QIcon>(data).pixmap(64, 64);
    }

    const QFont oldFont = painter->font();
    QFont boldFont(oldFont);
    boldFont.setBold(true);
    painter->setFont(boldFont);
    QFontMetrics fm = painter->fontMetrics();
    int hn = fm.boundingRect(0, 0, 0, 0, Qt::AlignLeft, name).height();
    int wn = fm.boundingRect(0, 0, 0, 0, Qt::AlignLeft, name).width();
    painter->setFont(oldFont);

    fm = painter->fontMetrics();
    int hc = fm.boundingRect(0, 0, 0, 0, Qt::AlignLeft, comment).height();
    int wc = fm.boundingRect(0, 0, 0, 0, Qt::AlignLeft, comment).width();
    int wp = pixmap.width();

    QStyleOptionViewItem opt(option);
    opt.showDecorationSelected = true;
    QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter);

    QPen pen = painter->pen();
    QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(option.state & QStyle::State_Active)) {
        cg = QPalette::Inactive;
    }
    if (option.state & QStyle::State_Selected) {
        painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
    } else {
        painter->setPen(option.palette.color(cg, QPalette::Text));
    }

    painter->setFont(option.font);

    painter->drawPixmap(option.rect.x() + 5, option.rect.y() + 5, pixmap);

    painter->setFont(boldFont);
    if (!name.isEmpty()) {
        painter->drawText(option.rect.x() + 5 + wp + 5, option.rect.y() + 7, wn, hn, Qt::AlignLeft, name);
    }
    painter->setFont(oldFont);

    if (!comment.isEmpty()) {
        painter->drawText(option.rect.x() + 5 + wp + 5, option.rect.y() + 7 + hn, wc, hc, Qt::AlignLeft, comment);
    }

    painter->setPen(pen);

    drawFocus(painter, option, option.rect);
}

QSize AgentTypeWidgetDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QSize(0, 0);
    }

    const QString name = index.model()->data(index, Qt::DisplayRole).toString();
    const QString comment = index.model()->data(index, AgentTypeModel::DescriptionRole).toString();

    QFontMetrics fm = option.fontMetrics;
    int hn = fm.boundingRect(0, 0, 0, 0, Qt::AlignLeft, name).height();
    int wn = fm.boundingRect(0, 0, 0, 0, Qt::AlignLeft, name).width();
    int hc = fm.boundingRect(0, 0, 0, 0, Qt::AlignLeft, comment).height();
    int wc = fm.boundingRect(0, 0, 0, 0, Qt::AlignLeft, comment).width();

    int width = 0;
    int height = 0;

    if (!name.isEmpty()) {
        height += hn;
        width = qMax(width, wn);
    }

    if (!comment.isEmpty()) {
        height += hc;
        width = qMax(width, wc);
    }

    height = qMax(height, 64) + 10;
    width += 64 + 15;

    return QSize(width, height);
}

void AgentTypeWidgetDelegate::drawFocus(QPainter *painter, const QStyleOptionViewItem &option, QRect rect) const
{
    if (option.state & QStyle::State_HasFocus) {
        QStyleOptionFocusRect o;
        o.QStyleOption::operator=(option);
        o.rect = rect;
        o.state |= QStyle::State_KeyboardFocusChange;
        QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
        o.backgroundColor = option.palette.color(cg, (option.state & QStyle::State_Selected) ? QPalette::Highlight : QPalette::Window);
        QApplication::style()->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter);
    }
}

} // namespace Akonadi

#include "agenttypewidget.moc"

#include "moc_agenttypewidget.cpp"
