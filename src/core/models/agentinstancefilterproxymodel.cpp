/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentinstancefilterproxymodel.h"

#include "accountactivitiesabstract.h"
#include "agentinstancemodel.h"

#include <QMimeDatabase>
#include <QMimeType>
#include <QStringList>

using namespace Akonadi;
/**
 * @internal
 */
class Akonadi::AgentInstanceFilterProxyModelPrivate
{
public:
    QStringList mimeTypes;
    QStringList capabilities;
    QStringList excludeCapabilities;
    bool enablePlasmaActivities = false;
    AccountActivitiesAbstract *accountActivitiesAbstract = nullptr;
    [[nodiscard]] bool filterAcceptRegExp(const QModelIndex &index, const QRegularExpression &filterRegExpStr);
};

AgentInstanceFilterProxyModel::AgentInstanceFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d(new AgentInstanceFilterProxyModelPrivate)
{
}

AgentInstanceFilterProxyModel::~AgentInstanceFilterProxyModel() = default;

void AgentInstanceFilterProxyModel::addMimeTypeFilter(const QString &mimeType)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    beginFilterChange();
#endif
    d->mimeTypes << mimeType;
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
    invalidateFilter();
#endif
}

void AgentInstanceFilterProxyModel::addCapabilityFilter(const QString &capability)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    beginFilterChange();
#endif
    d->capabilities << capability;
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
    invalidateFilter();
#endif
}

void AgentInstanceFilterProxyModel::excludeCapabilities(const QString &capability)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    beginFilterChange();
#endif
    d->excludeCapabilities << capability;
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
    invalidateFilter();
#endif
}

bool AgentInstanceFilterProxyModel::enablePlasmaActivities() const
{
    return d->enablePlasmaActivities;
}

void AgentInstanceFilterProxyModel::setEnablePlasmaActivities(bool newEnablePlasmaActivities)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    beginFilterChange();
#endif
    d->enablePlasmaActivities = newEnablePlasmaActivities;
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
    invalidateFilter();
#endif
}

void AgentInstanceFilterProxyModel::clearFilters()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    beginFilterChange();
#endif
    d->capabilities.clear();
    d->mimeTypes.clear();
    d->excludeCapabilities.clear();
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
    invalidateFilter();
#endif
}

bool AgentInstanceFilterProxyModelPrivate::filterAcceptRegExp(const QModelIndex &index, const QRegularExpression &filterRegExpStr)
{
    // First see if the name matches a set regexp filter.
    if (!filterRegExpStr.pattern().isEmpty()) {
        return index.data(AgentInstanceModel::TypeIdentifierRole).toString().contains(filterRegExpStr) || index.data().toString().contains(filterRegExpStr);
    }
    return true;
}

bool AgentInstanceFilterProxyModel::filterAcceptsRow(int row, const QModelIndex & /*source_parent*/) const
{
    const QModelIndex index = sourceModel()->index(row, 0);

    if (!d->mimeTypes.isEmpty()) {
        QMimeDatabase mimeDb;
        bool found = false;
        const QStringList lst = index.data(AgentInstanceModel::MimeTypesRole).toStringList();
        for (const QString &mimeType : lst) {
            if (d->mimeTypes.contains(mimeType)) {
                found = true;
            } else {
                const QMimeType mt = mimeDb.mimeTypeForName(mimeType);
                if (mt.isValid()) {
                    for (const QString &type : std::as_const(d->mimeTypes)) {
                        if (mt.inherits(type)) {
                            found = true;
                            break;
                        }
                    }
                }
            }

            if (found) {
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    if (!d->capabilities.isEmpty()) {
        bool found = false;
        const QStringList lst = index.data(AgentInstanceModel::CapabilitiesRole).toStringList();
        for (const QString &capability : lst) {
            if (d->capabilities.contains(capability)) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }

        if (!d->excludeCapabilities.isEmpty()) {
            const QStringList lstCapabilities = index.data(AgentInstanceModel::CapabilitiesRole).toStringList();
            for (const QString &capability : lstCapabilities) {
                if (d->excludeCapabilities.contains(capability)) {
                    found = false;
                    break;
                }
            }

            if (!found) {
                return false;
            }
        }
    }
    const bool result = d->filterAcceptRegExp(index, filterRegularExpression());
    if (result) {
        if (d->accountActivitiesAbstract && d->enablePlasmaActivities) {
            const bool enableActivities = index.data(AgentInstanceModel::ActivitiesEnabledRole).toBool();
            if (enableActivities) {
                const auto activities = index.data(AgentInstanceModel::ActivitiesRole).toStringList();
                const bool result = d->accountActivitiesAbstract->filterAcceptsRow(activities);
                // qDebug() << " result " << result << " identity name : " << sourceModel()->index(source_row,
                // IdentityTreeModel::IdentityNameRole).data().toString();
                return result;
            }
        }
    }
    return result;
}

AccountActivitiesAbstract *AgentInstanceFilterProxyModel::accountActivitiesAbstract() const
{
    return d->accountActivitiesAbstract;
}

void AgentInstanceFilterProxyModel::setAccountActivitiesAbstract(AccountActivitiesAbstract *abstract)
{
    if (d->accountActivitiesAbstract != abstract) {
        d->accountActivitiesAbstract = abstract;
        connect(d->accountActivitiesAbstract, &AccountActivitiesAbstract::activitiesChanged, this, &AgentInstanceFilterProxyModel::slotInvalidateFilter);
        slotInvalidateFilter();
    }
}

void AgentInstanceFilterProxyModel::slotInvalidateFilter()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    beginFilterChange();
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
    invalidateFilter();
#endif
}

#include "moc_agentinstancefilterproxymodel.cpp"
