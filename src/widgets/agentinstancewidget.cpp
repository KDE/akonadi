/*
    Copyright (c) 2006-2008 Tobias Koenig <tokoe@kde.org>

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

#include "agentinstancewidget.h"

#include "agentfilterproxymodel.h"
#include "agentinstance.h"
#include "agentinstancemodel.h"

#include <QIcon>
#include <KIconLoader>

#include <QtCore/QUrl>
#include <QApplication>
#include <QHBoxLayout>
#include <QListView>
#include <QPainter>

namespace Akonadi {
namespace Internal {

static void iconsEarlyCleanup();

struct Icons
{
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

Q_GLOBAL_STATIC(Icons, s_icons)

// called as a Qt post routine, to prevent pixmap leaking
void iconsEarlyCleanup() {
    Icons *const ic = s_icons;
    ic->readyPixmap = ic->syncPixmap = ic->errorPixmap = ic->offlinePixmap = QPixmap();
}

static const int s_delegatePaddingSize = 7;

/**
 * @internal
 */

class AgentInstanceWidgetDelegate : public QAbstractItemDelegate
{
public:
    AgentInstanceWidgetDelegate(QObject *parent = Q_NULLPTR);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
};

}

using Akonadi::Internal::AgentInstanceWidgetDelegate;

/**
 * @internal
 */
class AgentInstanceWidget::Private
{
public:
    Private(AgentInstanceWidget *parent)
        : mParent(parent)
        , mView(0)
        , mModel(0)
        , proxy(0)
    {
    }

    void currentAgentInstanceChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex);
    void currentAgentInstanceDoubleClicked(const QModelIndex &currentIndex);
    void currentAgentInstanceClicked(const QModelIndex &currentIndex);

    AgentInstanceWidget *mParent;
    QListView *mView;
    AgentInstanceModel *mModel;
    AgentFilterProxyModel *proxy;
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

    emit mParent->currentChanged(currentInstance, previousInstance);
}

void AgentInstanceWidget::Private::currentAgentInstanceDoubleClicked(const QModelIndex &currentIndex)
{
    AgentInstance currentInstance;
    if (currentIndex.isValid()) {
        currentInstance = currentIndex.data(AgentInstanceModel::InstanceRole).value<AgentInstance>();
    }

    emit mParent->doubleClicked(currentInstance);
}

void AgentInstanceWidget::Private::currentAgentInstanceClicked(const QModelIndex &currentIndex)
{
    AgentInstance currentInstance;
    if (currentIndex.isValid()) {
        currentInstance = currentIndex.data(AgentInstanceModel::InstanceRole).value<AgentInstance>();
    }

    emit mParent->clicked(currentInstance);
}

AgentInstanceWidget::AgentInstanceWidget(QWidget *parent)
    : QWidget(parent)
    , d(new Private(this))
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);

    d->mView = new QListView(this);
    d->mView->setContextMenuPolicy(Qt::NoContextMenu);
    d->mView->setItemDelegate(new Internal::AgentInstanceWidgetDelegate(d->mView));
    d->mView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    d->mView->setAlternatingRowColors(true);
    d->mView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    layout->addWidget(d->mView);

    d->mModel = new AgentInstanceModel(this);

    d->proxy = new AgentFilterProxyModel(this);
    d->proxy->setSourceModel(d->mModel);
    d->mView->setModel(d->proxy);

    d->mView->selectionModel()->setCurrentIndex(d->mView->model()->index(0, 0), QItemSelectionModel::Select);
    d->mView->scrollTo(d->mView->model()->index(0, 0));

    connect(d->mView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(currentAgentInstanceChanged(QModelIndex,QModelIndex)));
    connect(d->mView, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(currentAgentInstanceDoubleClicked(QModelIndex)));
    connect(d->mView, SIGNAL(clicked(QModelIndex)),
            this, SLOT(currentAgentInstanceClicked(QModelIndex)));
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

QList<AgentInstance> AgentInstanceWidget::selectedAgentInstances() const
{
    QList<AgentInstance> list;
    QItemSelectionModel *selectionModel = d->mView->selectionModel();
    if (!selectionModel) {
        return list;
    }

    const QModelIndexList indexes = selectionModel->selection().indexes();

    foreach (const QModelIndex &index, indexes) {
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
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, 0);

    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
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
        statusMessage.append(QString::fromLatin1(" (%1%)").arg(progress));
    }

    QRect innerRect = option.rect.adjusted(s_delegatePaddingSize, s_delegatePaddingSize, -s_delegatePaddingSize, -s_delegatePaddingSize);   //add some padding round entire delegate

    const QSize decorationSize(KIconLoader::global()->currentSize(KIconLoader::Desktop), KIconLoader::global()->currentSize(KIconLoader::Desktop));
    const QSize statusIconSize = QSize(16, 16); //= KIconLoader::global()->currentSize(KIconLoader::Small);

    QFont nameFont = option.font;
    nameFont.setBold(true);

    QFont statusTextFont = option.font;
    const QRect decorationRect(innerRect.left(), innerRect.top(), decorationSize.width(), innerRect.height());
    const QRect nameTextRect(decorationRect.topRight() + QPoint(4, 0), innerRect.topRight() + QPoint(0, innerRect.height() / 2));
    const QRect statusTextRect(decorationRect.bottomRight() + QPoint(4, - innerRect.height() / 2), innerRect.bottomRight());

    const QPixmap iconPixmap = icon.pixmap(decorationSize);

    QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
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
    Q_UNUSED(index);

    const int iconHeight = KIconLoader::global()->currentSize(KIconLoader::Desktop) + (s_delegatePaddingSize * 2);  //icon height + padding either side
    const int textHeight = option.fontMetrics.height() + qMax(option.fontMetrics.height(), 16) + (s_delegatePaddingSize * 2);   //height of text + icon/text + padding either side

    return QSize(1, qMax(iconHeight, textHeight));    //any width,the view will give us the whole thing in list mode
}

}

#include "moc_agentinstancewidget.cpp"
