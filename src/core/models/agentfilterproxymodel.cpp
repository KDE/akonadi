/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "agentfilterproxymodel.h"

#include "agenttypemodel.h"
#include "agentinstancemodel.h"


#include <QDebug>

#include <QStringList>
#include <QMimeDatabase>
#include <QMimeType>

using namespace Akonadi;

// ensure the role numbers are equivalent for both source models
static_assert((int)AgentTypeModel::CapabilitiesRole == (int)AgentInstanceModel::CapabilitiesRole,
              "AgentTypeModel::CapabilitiesRole does not match AgentInstanceModel::CapabilitiesRole");
static_assert((int)AgentTypeModel::MimeTypesRole == (int)AgentInstanceModel::MimeTypesRole,
              "AgentTypeModel::MimeTypesRole does not match AgentInstanceModel::MimeTypesRole");

/**
 * @internal
 */
class Q_DECL_HIDDEN AgentFilterProxyModel::Private
{
public:
    QStringList mimeTypes;
    QStringList capabilities;
    QStringList excludeCapabilities;
    bool filterAcceptRegExp(const QModelIndex &index, const QRegExp &filterRegExpStr);
};

AgentFilterProxyModel::AgentFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d(new Private)
{
    setDynamicSortFilter(true);
}

AgentFilterProxyModel::~AgentFilterProxyModel()
{
    delete d;
}

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

void AgentFilterProxyModel::clearFilters()
{
    d->capabilities.clear();
    d->mimeTypes.clear();
    d->excludeCapabilities.clear();
    invalidateFilter();
}

bool AgentFilterProxyModel::Private::filterAcceptRegExp(const QModelIndex &index, const QRegExp &filterRegExpStr)
{
    // First see if the name matches a set regexp filter.
    if (!filterRegExpStr.isEmpty()) {
        if (index.data(AgentTypeModel::IdentifierRole).toString().contains(filterRegExpStr)) {
            return true;
        } else if (index.data().toString().contains(filterRegExpStr)) {
            return true;
        } else {
            return false;
        }
    }
    return true;
}

bool AgentFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &) const
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
                    for (const QString &type : qAsConst(d->mimeTypes)) {
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

        if (found && !d->excludeCapabilities.isEmpty()) {
            const QStringList lst = index.data(AgentTypeModel::CapabilitiesRole).toStringList();
            for (const QString &capability : lst) {
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

    return d->filterAcceptRegExp(index, filterRegExp());
}
