/*
    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agenttypemodel.h"
#include "agentmanager.h"
#include "agenttype.h"

#include <QIcon>

using namespace Akonadi;

/**
 * @internal
 */
class Q_DECL_HIDDEN AgentTypeModel::Private
{
public:
    explicit Private(AgentTypeModel *parent)
        : mParent(parent)
    {
        mTypes = AgentManager::self()->types();
    }

    AgentTypeModel *const mParent;
    AgentType::List mTypes;

    void typeAdded(const AgentType &agentType);
    void typeRemoved(const AgentType &agentType);
};

void AgentTypeModel::Private::typeAdded(const AgentType &agentType)
{
    mTypes.append(agentType);

    Q_EMIT mParent->layoutChanged();
}

void AgentTypeModel::Private::typeRemoved(const AgentType &agentType)
{
    mTypes.removeAll(agentType);

    Q_EMIT mParent->layoutChanged();
}

AgentTypeModel::AgentTypeModel(QObject *parent)
    : QAbstractItemModel(parent)
    , d(new Private(this))
{
    connect(AgentManager::self(), &AgentManager::typeAdded, this, [this](const Akonadi::AgentType &type) {
        d->typeAdded(type);
    });
    connect(AgentManager::self(), &AgentManager::typeRemoved, this, [this](const Akonadi::AgentType &type) {
        d->typeRemoved(type);
    });
}

AgentTypeModel::~AgentTypeModel() = default;

int AgentTypeModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 1;
}

int AgentTypeModel::rowCount(const QModelIndex & /*parent*/) const
{
    return d->mTypes.count();
}

QHash<int, QByteArray> AgentTypeModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles.insert(TypeRole, QByteArrayLiteral("type"));
    roles.insert(IdentifierRole, QByteArrayLiteral("identifier"));
    roles.insert(DescriptionRole, QByteArrayLiteral("description"));
    roles.insert(MimeTypesRole, QByteArrayLiteral("mimeTypes"));
    roles.insert(CapabilitiesRole, QByteArrayLiteral("capabilities"));
    return roles;
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

QModelIndex AgentTypeModel::index(int row, int column, const QModelIndex & /*parent*/) const
{
    if (row < 0 || row >= d->mTypes.count()) {
        return QModelIndex();
    }

    if (column != 0) {
        return QModelIndex();
    }

    return createIndex(row, column);
}

QModelIndex AgentTypeModel::parent(const QModelIndex & /*child*/) const
{
    return QModelIndex();
}

Qt::ItemFlags AgentTypeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= d->mTypes.count()) {
        return QAbstractItemModel::flags(index);
    }

    const AgentType &type = d->mTypes[index.row()];
    if (type.capabilities().contains(QLatin1String("Unique")) && AgentManager::self()->instance(type.identifier()).isValid()) {
        return QAbstractItemModel::flags(index) & ~(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    }
    return QAbstractItemModel::flags(index);
}

#include "moc_agenttypemodel.cpp"
