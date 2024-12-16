/*
    SPDX-FileCopyrightText: 2009 Kevin Ottens <ervin@kde.org>
                  2016 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "statisticsproxymodel.h"
#include "akonadicore_debug.h"

#include "collectionquotaattribute.h"
#include "collectionstatistics.h"
#include "collectionutils.h"
#include "entitydisplayattribute.h"
#include "entitytreemodel.h"

#include <KFormat>
#include <KIconLoader>
#include <KLocalizedString>

#include <QApplication>
#include <QMetaMethod>
#include <QPalette>

using namespace Akonadi;

/**
 * @internal
 */
class Akonadi::StatisticsProxyModelPrivate
{
public:
    explicit StatisticsProxyModelPrivate(StatisticsProxyModel *parent)
        : q(parent)
    {
    }

    void getCountRecursive(const QModelIndex &index, qint64 &totalSize) const
    {
        auto collection = qvariant_cast<Collection>(index.data(EntityTreeModel::CollectionRole));
        // Do not assert on invalid collections, since a collection may be deleted
        // in the meantime and deleted collections are invalid.
        if (collection.isValid()) {
            CollectionStatistics statistics = collection.statistics();
            totalSize += qMax(0LL, statistics.size());
            if (index.model()->hasChildren(index)) {
                const int rowCount = index.model()->rowCount(index);
                for (int row = 0; row < rowCount; row++) {
                    static const int column = 0;
                    getCountRecursive(index.model()->index(row, column, index), totalSize);
                }
            }
        }
    }

    int sourceColumnCount() const
    {
        return q->sourceModel()->columnCount();
    }

    QString toolTipForCollection(const QModelIndex &index, const Collection &collection) const
    {
        const QString bckColor = QApplication::palette().color(QPalette::ToolTipBase).name();
        const QString txtColor = QApplication::palette().color(QPalette::ToolTipText).name();

        QString tip = QStringLiteral("<table width=\"100%\" border=\"0\" cellpadding=\"2\" cellspacing=\"0\">\n");
        const QString textDirection = (QApplication::layoutDirection() == Qt::LeftToRight) ? QStringLiteral("left") : QStringLiteral("right");
        tip += QStringLiteral(
                   "  <tr>\n"
                   "    <td bgcolor=\"%1\" colspan=\"2\" align=\"%4\" valign=\"middle\">\n"
                   "      <div style=\"color: %2; font-weight: bold;\">\n"
                   "      %3\n"
                   "      </div>\n"
                   "    </td>\n"
                   "  </tr>\n")
                   .arg(txtColor, bckColor, index.data(Qt::DisplayRole).toString(), textDirection);

        tip += QStringLiteral(
                   "  <tr>\n"
                   "    <td align=\"%1\" valign=\"top\">\n")
                   .arg(textDirection);

        KFormat format;
        QString tipInfo = QStringLiteral(
                              "      <strong>%1</strong>: %2<br>\n"
                              "      <strong>%3</strong>: %4<br><br>\n")
                              .arg(i18n("Total Messages"))
                              .arg(collection.statistics().count())
                              .arg(i18n("Unread Messages"))
                              .arg(collection.statistics().unreadCount());

        if (collection.hasAttribute<CollectionQuotaAttribute>()) {
            const auto quota = collection.attribute<CollectionQuotaAttribute>();
            if (quota->currentValue() > -1 && quota->maximumValue() > 0) {
                qreal percentage = (100.0 * quota->currentValue()) / quota->maximumValue();
                QString percentStr = QString::number(percentage, 'f', 2);
                tipInfo += i18nc("@info:tooltip Quota: 10% (300 MiB/3 GiB)",
                                 "<strong>Quota</strong>: %1% (%2/%3)<br>\n",
                                 percentStr,
                                 format.formatByteSize(quota->currentValue()),
                                 format.formatByteSize(quota->maximumValue()));
            }
        }

        qint64 currentFolderSize(collection.statistics().size());
        tipInfo += QStringLiteral("      <strong>%1</strong>: %2<br>\n").arg(i18n("Storage Size"), format.formatByteSize(currentFolderSize));

        qint64 totalSize = 0;
        getCountRecursive(index, totalSize);
        totalSize -= currentFolderSize;
        if (totalSize > 0) {
            tipInfo += QStringLiteral("<strong>%1</strong>: %2<br>").arg(i18n("Subfolder Storage Size"), format.formatByteSize(totalSize));
        }

        QString iconName = CollectionUtils::defaultIconName(collection);
        if (collection.hasAttribute<EntityDisplayAttribute>() && !collection.attribute<EntityDisplayAttribute>()->iconName().isEmpty()) {
            if (!collection.attribute<EntityDisplayAttribute>()->activeIconName().isEmpty() && collection.statistics().unreadCount() > 0) {
                iconName = collection.attribute<EntityDisplayAttribute>()->activeIconName();
            } else {
                iconName = collection.attribute<EntityDisplayAttribute>()->iconName();
            }
        }

        int iconSizes[] = {32, 22};
        int icon_size_found = 32;

        QString iconPath;

        for (int i = 0; i < 2; ++i) {
            iconPath = KIconLoader::global()->iconPath(iconName, -iconSizes[i], true);
            if (!iconPath.isEmpty()) {
                icon_size_found = iconSizes[i];
                break;
            }
        }

        if (iconPath.isEmpty()) {
            iconPath = KIconLoader::global()->iconPath(QStringLiteral("folder"), -32, false);
        }

        const QString tipIcon = QStringLiteral(
                                    "      <table border=\"0\"><tr><td width=\"32\" height=\"32\" align=\"center\" valign=\"middle\">\n"
                                    "      <img src=\"%1\" width=\"%2\" height=\"32\">\n"
                                    "      </td></tr></table>\n"
                                    "    </td>\n")
                                    .arg(iconPath)
                                    .arg(icon_size_found);

        if (QApplication::layoutDirection() == Qt::LeftToRight) {
            tip += tipInfo + QStringLiteral("</td><td align=\"%3\" valign=\"top\">").arg(textDirection) + tipIcon;
        } else {
            tip += tipIcon + QStringLiteral("</td><td align=\"%3\" valign=\"top\">").arg(textDirection) + tipInfo;
        }

        tip += QLatin1StringView(
            "  </tr>"
            "</table>");

        return tip;
    }

    void _k_sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles);

    StatisticsProxyModel *const q;

    bool mToolTipEnabled = false;
    bool mExtraColumnsEnabled = false;
};

void StatisticsProxyModelPrivate::_k_sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    QModelIndex proxyTopLeft(q->mapFromSource(topLeft));
    QModelIndex proxyBottomRight(q->mapFromSource(bottomRight));
    // Emit data changed for the whole row (bug #222292)
    if (mExtraColumnsEnabled && topLeft.column() == 0) { // in theory we could filter on roles, but ETM doesn't set any yet
        const int lastColumn = q->columnCount() - 1;
        proxyBottomRight = proxyBottomRight.sibling(proxyBottomRight.row(), lastColumn);
    }
    Q_EMIT q->dataChanged(proxyTopLeft, proxyBottomRight, roles);
}

void StatisticsProxyModel::setSourceModel(QAbstractItemModel *model)
{
    if (sourceModel()) {
        disconnect(sourceModel(), &QAbstractItemModel::dataChanged, this, nullptr);
    }
    KExtraColumnsProxyModel::setSourceModel(model);
    if (model) {
#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
        // Disconnect the default handling of dataChanged in QIdentityProxyModel, so we can extend it to the whole row
        disconnect(model,
                   SIGNAL(dataChanged(QModelIndex, QModelIndex, QList<int>)), // clazy:exclude=old-style-connect
                   this,
                   SLOT(_q_sourceDataChanged(QModelIndex, QModelIndex, QList<int>)));
#endif
        connect(model, &QAbstractItemModel::dataChanged, this, [this](const auto &tl, const auto &br, const auto &roles) {
            d->_k_sourceDataChanged(tl, br, roles);
        });
    }
}

StatisticsProxyModel::StatisticsProxyModel(QObject *parent)
    : KExtraColumnsProxyModel(parent)
    , d(new StatisticsProxyModelPrivate(this))
{
    setExtraColumnsEnabled(true);
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    // Disable the default handling of dataChanged in QIdentityProxyModel, so we can extend it to the whole row
    setHandleSourceDataChanges(false);
#endif
}

StatisticsProxyModel::~StatisticsProxyModel() = default;

void StatisticsProxyModel::setToolTipEnabled(bool enable)
{
    d->mToolTipEnabled = enable;
}

bool StatisticsProxyModel::isToolTipEnabled() const
{
    return d->mToolTipEnabled;
}

void StatisticsProxyModel::setExtraColumnsEnabled(bool enable)
{
    if (d->mExtraColumnsEnabled == enable) {
        return;
    }
    d->mExtraColumnsEnabled = enable;
    if (enable) {
        KExtraColumnsProxyModel::appendColumn(i18nc("number of unread entities in the collection", "Unread"));
        KExtraColumnsProxyModel::appendColumn(i18nc("number of entities in the collection", "Total"));
        KExtraColumnsProxyModel::appendColumn(i18nc("collection size", "Size"));
    } else {
        KExtraColumnsProxyModel::removeExtraColumn(2);
        KExtraColumnsProxyModel::removeExtraColumn(1);
        KExtraColumnsProxyModel::removeExtraColumn(0);
    }
}

bool StatisticsProxyModel::isExtraColumnsEnabled() const
{
    return d->mExtraColumnsEnabled;
}

QVariant StatisticsProxyModel::extraColumnData(const QModelIndex &parent, int row, int extraColumn, int role) const
{
    switch (role) {
    case Qt::DisplayRole: {
        const QModelIndex firstColumn = index(row, 0, parent);
        const auto collection = data(firstColumn, EntityTreeModel::CollectionRole).value<Collection>();
        if (collection.isValid() && collection.statistics().count() >= 0) {
            const CollectionStatistics stats = collection.statistics();
            if (extraColumn == 2) {
                KFormat format;
                return format.formatByteSize(stats.size());
            } else if (extraColumn == 1) {
                return stats.count();
            } else if (extraColumn == 0) {
                if (stats.unreadCount() > 0) {
                    return stats.unreadCount();
                } else {
                    return QString();
                }
            } else {
                qCWarning(AKONADICORE_LOG) << "We shouldn't get there for a column which is not total, unread or size.";
            }
        }
    } break;
    case Qt::TextAlignmentRole: {
        return Qt::AlignRight;
    }
    default:
        break;
    }
    return QVariant();
}

QVariant StatisticsProxyModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::ToolTipRole && d->mToolTipEnabled) {
        const QModelIndex firstColumn = index.sibling(index.row(), 0);
        const auto collection = data(firstColumn, EntityTreeModel::CollectionRole).value<Collection>();

        if (collection.isValid()) {
            return d->toolTipForCollection(firstColumn, collection);
        }
    }

    return KExtraColumnsProxyModel::data(index, role);
}

Qt::ItemFlags StatisticsProxyModel::flags(const QModelIndex &index_) const
{
    if (sourceModel() && index_.column() >= d->sourceColumnCount()) {
        const QModelIndex firstColumn = index_.sibling(index_.row(), 0);
        return KExtraColumnsProxyModel::flags(firstColumn)
            & (Qt::ItemIsSelectable | Qt::ItemIsDragEnabled // Allowed flags
               | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);
    }

    return KExtraColumnsProxyModel::flags(index_);
}

// Not sure this is still necessary....
QModelIndexList StatisticsProxyModel::match(const QModelIndex &start, int role, const QVariant &value, int hits, Qt::MatchFlags flags) const
{
    if (role < Qt::UserRole) {
        return KExtraColumnsProxyModel::match(start, role, value, hits, flags);
    }

    QModelIndexList list;
    QModelIndex proxyIndex;
    const auto matches = sourceModel()->match(mapToSource(start), role, value, hits, flags);
    for (const auto &idx : matches) {
        proxyIndex = mapFromSource(idx);
        if (proxyIndex.isValid()) {
            list.push_back(proxyIndex);
        }
    }

    return list;
}

#include "moc_statisticsproxymodel.cpp"
