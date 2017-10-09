/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

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

#include "agenttypemodel.h"
#include "agenttype.h"
#include "agentmanager.h"

#include <QIcon>

using namespace Akonadi;

/**
 * @internal
 */
class Q_DECL_HIDDEN AgentTypeModel::Private
{
public:
    Private(AgentTypeModel *parent)
        : mParent(parent)
    {
        mTypes = AgentManager::self()->types();
    }

    AgentTypeModel *mParent = nullptr;
    AgentType::List mTypes;

    void typeAdded(const AgentType &agentType);
    void typeRemoved(const AgentType &agentType);
};

void AgentTypeModel::Private::typeAdded(const AgentType &agentType)
{
    mTypes.append(agentType);

    emit mParent->layoutChanged();
}

void AgentTypeModel::Private::typeRemoved(const AgentType &agentType)
{
    mTypes.removeAll(agentType);

    emit mParent->layoutChanged();
}

AgentTypeModel::AgentTypeModel(QObject *parent)
    : QAbstractItemModel(parent)
    , d(new Private(this))
{
    connect(AgentManager::self(), &AgentManager::typeAdded, this, [this](const Akonadi::AgentType &type ) { d->typeAdded(type); });
    connect(AgentManager::self(), &AgentManager::typeRemoved, this, [this](const Akonadi::AgentType &type ) { d->typeRemoved(type); });
}

AgentTypeModel::~AgentTypeModel()
{
    delete d;
}

int AgentTypeModel::columnCount(const QModelIndex &) const
{
    return 1;
}

int AgentTypeModel::rowCount(const QModelIndex &) const
{
    return d->mTypes.count();
}

QVariant AgentTypeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() < 0 || index.row() >= d->mTypes.count()) {
        return QVariant();
    }

    const AgentType &type = d->mTypes[index.row()];

    switch (role) {
    case Qt::DisplayRole:
        return type.name();
    case Qt::DecorationRole:
        return type.icon();
    case TypeRole: {
        QVariant var;
        var.setValue(type);
        return var;
    }
    case IdentifierRole:
        return type.identifier();
    case DescriptionRole:
        return type.description();
    case MimeTypesRole:
        return type.mimeTypes();
    case CapabilitiesRole:
        return type.capabilities();
    default:
        break;
    }
    return QVariant();
}

QModelIndex AgentTypeModel::index(int row, int column, const QModelIndex &) const
{
    if (row < 0 || row >= d->mTypes.count()) {
        return QModelIndex();
    }

    if (column != 0) {
        return QModelIndex();
    }

    return createIndex(row, column);
}

QModelIndex AgentTypeModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

Qt::ItemFlags AgentTypeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= d->mTypes.count()) {
        return QAbstractItemModel::flags(index);
    }

    const AgentType &type = d->mTypes[index.row()];
    if (type.capabilities().contains(QStringLiteral("Unique")) &&
            AgentManager::self()->instance(type.identifier()).isValid()) {
        return QAbstractItemModel::flags(index) & ~(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    }
    return QAbstractItemModel::flags(index);
}

#include "moc_agenttypemodel.cpp"
