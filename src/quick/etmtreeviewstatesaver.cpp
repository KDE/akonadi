// SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
// SPDX-FileContributor: Stephen Kelly <stephen@kdab.com>
// SPDX-FileCopyrightText: 2024 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "etmtreeviewstatesaver.h"

#include <Akonadi/Collection>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/Item>

#include <KConfigGroup>
#include <KSharedConfig>

using namespace Qt::StringLiterals;
using namespace Qt::Literals::StringLiterals;
ETMTreeViewStateSaver::ETMTreeViewStateSaver(QObject *parent)
    : QObject(parent)
{
}

int ETMTreeViewStateSaver::currentIndex() const
{
    return m_currentIndex;
}

void ETMTreeViewStateSaver::setCurrentIndex(const int currentIndex, bool signal)
{
    if (m_currentIndex == currentIndex) {
        return;
    }
    m_currentIndex = currentIndex;
    if (signal) {
        Q_EMIT currentIndexChanged();
    } else {
        auto config = KSharedConfig::openStateConfig();
        auto group = config->group(m_configGroup);
        group.writeEntry(u"CurrentItem"_s, indexToConfigString(m_model->index(m_currentIndex, 0)));
        config->sync();
    }
}

QString ETMTreeViewStateSaver::configGroup() const
{
    return m_configGroup;
}

void ETMTreeViewStateSaver::setConfigGroup(const QString &configGroup)
{
    if (m_configGroup == configGroup) {
        return;
    }
    m_configGroup = configGroup;
    Q_EMIT configGroupChanged();
}

KDescendantsProxyModel *ETMTreeViewStateSaver::model() const
{
    return m_model;
}

void ETMTreeViewStateSaver::setModel(KDescendantsProxyModel *model)
{
    if (m_model == model) {
        return;
    }
    m_model = model;
    Q_EMIT modelChanged();

    if (!m_model) {
        return;
    }

    connect(m_model, &KDescendantsProxyModel::sourceIndexCollapsed, this, [this](const QModelIndex &index) {
        Q_UNUSED(index);
        saveState();
    });

    connect(m_model, &KDescendantsProxyModel::sourceIndexExpanded, this, [this](const QModelIndex &index) {
        Q_UNUSED(index);
        saveState();
    });

    if (m_model->sourceModel()) {
        restoreState();
    }
    connect(m_model, &KDescendantsProxyModel::sourceModelChanged, this, [this]() {
        if (m_model->sourceModel()) {
            restoreState();
        }
    });
}

QModelIndex ETMTreeViewStateSaver::indexFromConfigString(const QAbstractItemModel *model, const QString &key) const
{
    if (key.startsWith(u'x')) {
        return QModelIndex();
    }

    Akonadi::Item::Id id = key.mid(1).toLongLong();
    if (id < 0) {
        return QModelIndex();
    }

    if (key.startsWith(u'c')) {
        const QModelIndex idx = Akonadi::EntityTreeModel::modelIndexForCollection(model, Akonadi::Collection(id));
        return idx;
    } else if (key.startsWith(u'i')) {
        const QModelIndexList list = Akonadi::EntityTreeModel::modelIndexesForItem(model, Akonadi::Item(id));
        if (list.isEmpty()) {
            return QModelIndex();
        }
        return list.first();
    }
    return QModelIndex();
}

QString ETMTreeViewStateSaver::indexToConfigString(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return u"x-1"_s;
    }
    const auto c = index.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
    if (c.isValid()) {
        return u"c%1"_s.arg(c.id());
    }
    auto id = index.data(Akonadi::EntityTreeModel::ItemIdRole).value<Akonadi::Item::Id>();
    if (id >= 0) {
        return u"i%1"_s.arg(id);
    }
    return QString();
}

QStringList ETMTreeViewStateSaver::getExpandedItems(const QModelIndex &index) const
{
    const auto sourceModel = m_model->sourceModel();
    QStringList expansion;
    for (int i = 0; i < sourceModel->rowCount(index); ++i) {
        const QModelIndex child = sourceModel->index(i, 0, index);

        if (sourceModel->hasChildren(child)) {
            if (m_model->isSourceIndexExpanded(child)) {
                expansion << indexToConfigString(child);
            }
            expansion << getExpandedItems(child);
        }
    }
    return expansion;
}

bool ETMTreeViewStateSaver::hasPendingChanges() const
{
    return !m_pendingExpansions.isEmpty();
}

void ETMTreeViewStateSaver::listenToPendingChanges()
{
    if (!hasPendingChanges()) {
        return;
    }

    const QAbstractItemModel *model = m_model->sourceModel();
    if (model) {
        disconnect(m_rowsInsertedConnection);
        m_rowsInsertedConnection = connect(model, &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex &parent, int first, int last) {
            Q_UNUSED(parent);
            Q_UNUSED(first);
            Q_UNUSED(last);
            processPendingChanges();

            if (!hasPendingChanges()) {
                disconnect(m_rowsInsertedConnection);
            }
        });
        return;
    }
}

void ETMTreeViewStateSaver::restoreExpanded()
{
    QSet<QString>::iterator it = m_pendingExpansions.begin();
    for (; it != m_pendingExpansions.end();) {
        QModelIndex idx = indexFromConfigString(m_model->sourceModel(), *it);
        if (idx.isValid()) {
            m_model->expandSourceIndex(idx);
            it = m_pendingExpansions.erase(it);
        } else {
            ++it;
        }
    }
}

void ETMTreeViewStateSaver::restoreCurrentItem()
{
    QModelIndex currentIndex = indexFromConfigString(m_model->sourceModel(), m_pendingCurrent);
    if (currentIndex.isValid()) {
        setCurrentIndex(m_model->mapFromSource(currentIndex).row(), true);
        m_pendingCurrent.clear();
    }
}

void ETMTreeViewStateSaver::saveState()
{
    if (!m_model) {
        return;
    }

    auto config = KSharedConfig::openStateConfig();
    auto group = config->group(m_configGroup);
    QSet expansionItems = m_pendingExpansions; // not yet applied
    const auto newItems = getExpandedItems({});
    expansionItems.unite(QSet<QString>(newItems.begin(), newItems.end()));
    group.writeEntry(u"ExpandedItems"_s, expansionItems.values());
    group.writeEntry(u"CurrentItem"_s, indexToConfigString(m_model->index(m_currentIndex, 0)));
    config->sync();
}

void ETMTreeViewStateSaver::restoreState()
{
    auto config = KSharedConfig::openStateConfig();
    auto group = config->group(m_configGroup);
    const auto indexStrings = group.readEntry(u"ExpandedItems"_s, QStringList{});
    m_pendingExpansions.unite(QSet<QString>(indexStrings.begin(), indexStrings.end()));
    m_pendingCurrent = group.readEntry(u"CurrentItem"_s, QString{});

    processPendingChanges();
}

void ETMTreeViewStateSaver::processPendingChanges()
{
    restoreExpanded();
    restoreCurrentItem();

    if (hasPendingChanges()) {
        listenToPendingChanges();
    }
}

#include "moc_etmtreeviewstatesaver.cpp"
