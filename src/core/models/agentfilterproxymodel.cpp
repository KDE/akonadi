/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentfilterproxymodel.h"

#include "accountactivitiesabstract.h"
#include "agentinstancemodel.h"
#include "agenttypemodel.h"

#include <QMimeDatabase>
#include <QMimeType>
#include <QStringList>

using namespace Akonadi;

// ensure the role numbers are equivalent for both source models
static_assert(static_cast<int>(AgentTypeModel::CapabilitiesRole) == static_cast<int>(AgentInstanceModel::CapabilitiesRole),
              "AgentTypeModel::CapabilitiesRole does not match AgentInstanceModel::CapabilitiesRole");
static_assert(static_cast<int>(AgentTypeModel::MimeTypesRole) == static_cast<int>(AgentInstanceModel::MimeTypesRole),
              "AgentTypeModel::MimeTypesRole does not match AgentInstanceModel::MimeTypesRole");

/**
 * @internal
 */
class Akonadi::AgentFilterProxyModelPrivate
{
public:
    QStringList mimeTypes;
    QStringList capabilities;
    QStringList excludeCapabilities;
    bool enablePlasmaActivities = false;
    AccountActivitiesAbstract *accountActivitiesAbstract = nullptr;
    [[nodiscard]] bool filterAcceptRegExp(const QModelIndex &index, const QRegularExpression &filterRegExpStr);
};

AgentFilterProxyModel::AgentFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d(new AgentFilterProxyModelPrivate)
{
}

AgentFilterProxyModel::~AgentFilterProxyModel() = default;

void AgentFilterProxyModel::addMimeTypeFilter(const QString &mimeType)
{
    d->mimeTypes << mimeType;
    invalidateFilter();
}

void AgentFilterProxyModel::addCapabilityFilter(const QString &capability)
{
    d->capabilities << capability;
    invalidateFilter();
}

void AgentFilterProxyModel::excludeCapabilities(const QString &capability)
{
    d->excludeCapabilities << capability;
    invalidateFilter();
}

bool AgentFilterProxyModel::enablePlasmaActivities() const
{
    return d->enablePlasmaActivities;
}

void AgentFilterProxyModel::setEnablePlasmaActivities(bool newEnablePlasmaActivities)
{
    d->enablePlasmaActivities = newEnablePlasmaActivities;
    invalidateFilter();
}

void AgentFilterProxyModel::clearFilters()
{
    d->capabilities.clear();
    d->mimeTypes.clear();
    d->excludeCapabilities.clear();
    invalidateFilter();
}

bool AgentFilterProxyModelPrivate::filterAcceptRegExp(const QModelIndex &index, const QRegularExpression &filterRegExpStr)
{
    // First see if the name matches a set regexp filter.
    if (!filterRegExpStr.pattern().isEmpty()) {
        return index.data(AgentTypeModel::IdentifierRole).toString().contains(filterRegExpStr) || index.data().toString().contains(filterRegExpStr);
    }
    return true;
}

bool AgentFilterProxyModel::filterAcceptsRow(int row, const QModelIndex & /*source_parent*/) const
{
    const QModelIndex index = sourceModel()->index(row, 0);

    if (!d->mimeTypes.isEmpty()) {
        QMimeDatabase mimeDb;
        bool found = false;
        const QStringList lst = index.data(AgentTypeModel::MimeTypesRole).toStringList();
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
        const QStringList lst = index.data(AgentTypeModel::CapabilitiesRole).toStringList();
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
            const QStringList lstCapabilities = index.data(AgentTypeModel::CapabilitiesRole).toStringList();
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
#if 0
        const bool enableActivities = index.data(AgentInstanceModel::ActivitiesEnabledRole).toBool();
        if (enableActivities) {
            const auto activities = index.data(AgentInstanceModel::ActivitiesRole).toStringList();
            const bool result = d->accountActivitiesAbstract->filterAcceptsRow(activities);
            // qDebug() << " result " << result << " identity name : " << sourceModel()->index(source_row,
            // IdentityTreeModel::IdentityNameRole).data().toString();
            return result;
        }
#endif
        }
    }
    return result;
}

AccountActivitiesAbstract *AgentFilterProxyModel::accountActivitiesAbstract() const
{
    return d->accountActivitiesAbstract;
}

void AgentFilterProxyModel::setAccountActivitiesAbstract(AccountActivitiesAbstract *abstract)
{
    if (d->accountActivitiesAbstract != abstract) {
        d->accountActivitiesAbstract = abstract;
        connect(d->accountActivitiesAbstract, &AccountActivitiesAbstract::activitiesChanged, this, &AgentFilterProxyModel::invalidateFilter);
        invalidateFilter();
    }
}

#include "moc_agentfilterproxymodel.cpp"
