/*
    SPDX-FileCopyrightText: 2006-2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentinstancewidget.h"

#include "agentfilterproxymodel.h"
#include "agentinstance.h"
#include "agentinstancemodel.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QListView>
#include <QPainter>
#include <QStyle>

namespace Akonadi
{
namespace Internal
{
static void iconsEarlyCleanup();

struct Icons {
    Icons()
        : readyPixmap(QIcon::fromTheme(QStringLiteral("user-online")).pixmap(QSize(16, 16)))
        , syncPixmap(QIcon::fromTheme(QStringLiteral("network-connect")).pixmap(QSize(16, 16)))
        , errorPixmap(QIcon::fromTheme(QStringLiteral("dialog-error")).pixmap(QSize(16, 16)))
        , offlinePixmap(QIcon::fromTheme(QStringLiteral("network-disconnect")).pixmap(QSize(16, 16)))
    {
        qAddPostRoutine(iconsEarlyCleanup);
    }
    QPixmap readyPixmap, syncPixmap, errorPixmap, offlinePixmap;
};

Q_GLOBAL_STATIC(Icons, s_icons) // NOLINT(readability-redundant-member-init)

// called as a Qt post routine, to prevent pixmap leaking
void iconsEarlyCleanup()
{
    Icons *const ic = s_icons;
    ic->readyPixmap = ic->syncPixmap = ic->errorPixmap = ic->offlinePixmap = QPixmap();
}

static const int s_delegatePaddingSize = 7;

/**
 * @internal
 */

class AgentInstanceWidgetDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    explicit AgentInstanceWidgetDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

} // namespace Internal

using Akonadi::Internal::AgentInstanceWidgetDelegate;

/**
 * @internal
 */
class Q_DECL_HIDDEN AgentInstanceWidget::Private
{
public:
    explicit Private(AgentInstanceWidget *parent)
        : mParent(parent)
    {
    }

    void currentAgentInstanceChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex);
    void currentAgentInstanceDoubleClicked(const QModelIndex &currentIndex);
    void currentAgentInstanceClicked(const QModelIndex &currentIndex);

    AgentInstanceWidget *const mParent;
    QListView *mView = nullptr;
    AgentInstanceModel *mModel = nullptr;
    AgentFilterProxyModel *proxy = nullptr;
};

void AgentInstanceWidget::Private::currentAgentInstanceChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex)
{
    AgentInstance currentInstance;
    if (currentIndex.isValid()) {
        currentInstance = currentIndex.data(AgentInstanceModel::InstanceRole).value<AgentInstance>();
    }

    AgentInstance previousInstance;
    if (previousIndex.isValid()) {
        previousInstance = previousIndex.data(AgentInstanceModel::InstanceRole).value<AgentInstance>();
    }

    Q_EMIT mParent->currentChanged(currentInstance, previousInstance);
}

void AgentInstanceWidget::Private::currentAgentInstanceDoubleClicked(const QModelIndex &currentIndex)
{
    AgentInstance currentInstance;
    if (currentIndex.isValid()) {
        currentInstance = currentIndex.data(AgentInstanceModel::InstanceRole).value<AgentInstance>();
    }

    Q_EMIT mParent->doubleClicked(currentInstance);
}

void AgentInstanceWidget::Private::currentAgentInstanceClicked(const QModelIndex &currentIndex)
{
    AgentInstance currentInstance;
    if (currentIndex.isValid()) {
        currentInstance = currentIndex.data(AgentInstanceModel::InstanceRole).value<AgentInstance>();
    }

    Q_EMIT mParent->clicked(currentInstance);
}

AgentInstanceWidget::AgentInstanceWidget(QWidget *parent)
    : QWidget(parent)
    , d(new Private(this))
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    d->mView = new QListView(this);
    d->mView->setContextMenuPolicy(Qt::NoContextMenu);
    d->mView->setItemDelegate(new Internal::AgentInstanceWidgetDelegate(d->mView));
    d->mView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    d->mView->setAlternatingRowColors(true);
    d->mView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    layout->addWidget(d->mView);

    d->mModel = new AgentInstanceModel(this);

    d->proxy = new AgentFilterProxyModel(this);
    d->proxy->setDynamicSortFilter(true);
    d->proxy->sort(0);
    d->proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    d->proxy->setSourceModel(d->mModel);
    d->mView->setModel(d->proxy);

    d->mView->selectionModel()->setCurrentIndex(d->mView->model()->index(0, 0), QItemSelectionModel::Select);
    d->mView->scrollTo(d->mView->model()->index(0, 0));

    connect(d->mView->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const auto &tl, const auto &br) {
        d->currentAgentInstanceChanged(tl, br);
    });
    connect(d->mView, &QListView::doubleClicked, this, [this](const QModelIndex &currentIndex) {
        d->currentAgentInstanceDoubleClicked(currentIndex);
    });
    connect(d->mView, &QListView::clicked, this, [this](const auto &mi) {
        d->currentAgentInstanceClicked(mi);
    });
}

AgentInstanceWidget::~AgentInstanceWidget()
{
    delete d;
}

AgentInstance AgentInstanceWidget::currentAgentInstance() const
{
    QItemSelectionModel *selectionModel = d->mView->selectionModel();
    if (!selectionModel) {
        return AgentInstance();
    }

    QModelIndex index = selectionModel->currentIndex();
    if (!index.isValid()) {
        return AgentInstance();
    }

    return index.data(AgentInstanceModel::InstanceRole).value<AgentInstance>();
}

AgentInstance::List AgentInstanceWidget::selectedAgentInstances() const
{
    AgentInstance::List list;
    QItemSelectionModel *selectionModel = d->mView->selectionModel();
    if (!selectionModel) {
        return list;
    }

    const QModelIndexList indexes = selectionModel->selection().indexes();
    list.reserve(indexes.count());
    for (const QModelIndex &index : indexes) {
        list.append(index.data(AgentInstanceModel::InstanceRole).value<AgentInstance>());
    }

    return list;
}

QAbstractItemView *AgentInstanceWidget::view() const
{
    return d->mView;
}

AgentFilterProxyModel *AgentInstanceWidget::agentFilterProxyModel() const
{
    return d->proxy;
}

AgentInstanceWidgetDelegate::AgentInstanceWidgetDelegate(QObject *parent)
    : QAbstractItemDelegate(parent)
{
}

void AgentInstanceWidgetDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid()) {
        return;
    }

    QStyle *style = QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, nullptr);

    auto icon = index.data(Qt::DecorationRole).value<QIcon>();
    const QString name = index.model()->data(index, Qt::DisplayRole).toString();
    int status = index.model()->data(index, AgentInstanceModel::StatusRole).toInt();
    uint progress = index.model()->data(index, AgentInstanceModel::ProgressRole).toUInt();
    QString statusMessage = index.model()->data(index, AgentInstanceModel::StatusMessageRole).toString();

    QPixmap statusPixmap;

    if (!index.data(AgentInstanceModel::OnlineRole).toBool()) {
        statusPixmap = s_icons->offlinePixmap;
    } else if (status == AgentInstance::Idle) {
        statusPixmap = s_icons->readyPixmap;
    } else if (status == AgentInstance::Running) {
        statusPixmap = s_icons->syncPixmap;
    } else {
        statusPixmap = s_icons->errorPixmap;
    }

    if (status == 1) {
        statusMessage.append(QStringLiteral(" (%1%)").arg(progress));
    }

    const QPixmap iconPixmap = icon.pixmap(style->pixelMetric(QStyle::PM_MessageBoxIconSize));
    QRect innerRect = option.rect.adjusted(s_delegatePaddingSize,
                                           s_delegatePaddingSize,
                                           -s_delegatePaddingSize,
                                           -s_delegatePaddingSize); // add some padding round entire delegate

    const QSize decorationSize = iconPixmap.size();
    const QSize statusIconSize = statusPixmap.size(); //= KIconLoader::global()->currentSize(KIconLoader::Small);

    QFont nameFont = option.font;
    nameFont.setBold(true);

    QFont statusTextFont = option.font;
    const QRect decorationRect(innerRect.left(), innerRect.top(), decorationSize.width(), innerRect.height());
    const QRect nameTextRect(decorationRect.topRight() + QPoint(4, 0), innerRect.topRight() + QPoint(0, innerRect.height() / 2));
    const QRect statusTextRect(decorationRect.bottomRight() + QPoint(4, -innerRect.height() / 2), innerRect.bottomRight());

    QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(option.state & QStyle::State_Active)) {
        cg = QPalette::Inactive;
    }

    if (option.state & QStyle::State_Selected) {
        painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
    } else {
        painter->setPen(option.palette.color(cg, QPalette::Text));
    }

    painter->drawPixmap(style->itemPixmapRect(decorationRect, Qt::AlignCenter, iconPixmap), iconPixmap);

    painter->setFont(nameFont);
    painter->drawText(nameTextRect, Qt::AlignVCenter | Qt::AlignLeft, name);

    painter->setFont(statusTextFont);
    painter->drawText(statusTextRect.adjusted(statusIconSize.width() + 4, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, statusMessage);
    painter->drawPixmap(style->itemPixmapRect(statusTextRect, Qt::AlignVCenter | Qt::AlignLeft, statusPixmap), statusPixmap);
}

QSize AgentInstanceWidgetDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)

    const int iconHeight = QApplication::style()->pixelMetric(QStyle::PM_MessageBoxIconSize) + (s_delegatePaddingSize * 2); // icon height + padding either side
    const int textHeight =
        option.fontMetrics.height() + qMax(option.fontMetrics.height(), 16) + (s_delegatePaddingSize * 2); // height of text + icon/text + padding either side

    return QSize(1, qMax(iconHeight, textHeight)); // any width,the view will give us the whole thing in list mode
}

} // namespace Akonadi

#include "agentinstancewidget.moc"
