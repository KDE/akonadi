/*
    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentinstancemodel.h"

#include "agentinstance.h"
#include "agentmanager.h"

#include <QIcon>

#include <KLocalizedString>

using namespace Akonadi;

/**
 * @internal
 */
class Akonadi::AgentInstanceModelPrivate
{
public:
    explicit AgentInstanceModelPrivate(AgentInstanceModel *parent)
        : mParent(parent)
    {
    }

    AgentInstanceModel *const mParent;
    AgentInstance::List mInstances;

    void instanceAdded(const AgentInstance & /*instance*/);
    void instanceRemoved(const AgentInstance & /*instance*/);
    void instanceChanged(const AgentInstance & /*instance*/);
};

void AgentInstanceModelPrivate::instanceAdded(const AgentInstance &instance)
{
    mParent->beginInsertRows(QModelIndex(), mInstances.count(), mInstances.count());
    mInstances.append(instance);
    mParent->endInsertRows();
}

void AgentInstanceModelPrivate::instanceRemoved(const AgentInstance &instance)
{
    const int index = mInstances.indexOf(instance);
    if (index == -1) {
        return;
    }

    mParent->beginRemoveRows(QModelIndex(), index, index);
    mInstances.removeAll(instance);
    mParent->endRemoveRows();
}

void AgentInstanceModelPrivate::instanceChanged(const AgentInstance &instance)
{
    const int numberOfInstance(mInstances.count());
    for (int i = 0; i < numberOfInstance; ++i) {
        if (mInstances[i] == instance) {
            // TODO why reassign it if equals ?
            mInstances[i] = instance;

            const QModelIndex idx = mParent->index(i, 0);
            Q_EMIT mParent->dataChanged(idx, idx);

            return;
        }
    }
}

AgentInstanceModel::AgentInstanceModel(QObject *parent)
    : QAbstractItemModel(parent)
    , d(new AgentInstanceModelPrivate(this))
{
    d->mInstances = AgentManager::self()->instances();

    connect(AgentManager::self(), &AgentManager::instanceAdded, this, [this](const Akonadi::AgentInstance &inst) {
        d->instanceAdded(inst);
    });
    connect(AgentManager::self(), &AgentManager::instanceRemoved, this, [this](const Akonadi::AgentInstance &inst) {
        d->instanceRemoved(inst);
    });
    connect(AgentManager::self(), &AgentManager::instanceStatusChanged, this, [this](const Akonadi::AgentInstance &inst) {
        d->instanceChanged(inst);
    });
    connect(AgentManager::self(), &AgentManager::instanceProgressChanged, this, [this](const Akonadi::AgentInstance &inst) {
        d->instanceChanged(inst);
    });
    connect(AgentManager::self(), &AgentManager::instanceNameChanged, this, [this](const Akonadi::AgentInstance &inst) {
        d->instanceChanged(inst);
    });
    connect(AgentManager::self(), &AgentManager::instanceOnline, this, [this](const Akonadi::AgentInstance &inst) {
        d->instanceChanged(inst);
    });
}

AgentInstanceModel::~AgentInstanceModel() = default;

QHash<int, QByteArray> AgentInstanceModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles.insert(NameRole, QByteArrayLiteral("name"));
    roles.insert(StatusRole, QByteArrayLiteral("status"));
    roles.insert(StatusMessageRole, QByteArrayLiteral("statusMessage"));
    roles.insert(ProgressRole, QByteArrayLiteral("progress"));
    roles.insert(OnlineRole, QByteArrayLiteral("online"));
    roles.insert(IconNameRole, QByteArrayLiteral("iconName"));
    return roles;
}

int AgentInstanceModel::columnCount(const QModelIndex &index) const
{
    return index.isValid() ? 0 : 1;
}

int AgentInstanceModel::rowCount(const QModelIndex &index) const
{
    return index.isValid() ? 0 : d->mInstances.count();
}

QVariant AgentInstanceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() < 0 || index.row() >= d->mInstances.count()) {
        return QVariant();
    }

    const AgentInstance &instance = d->mInstances[index.row()];

    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return instance.name();
    case Qt::DecorationRole:
        return instance.type().icon();
    case IconNameRole:
        return instance.type().icon().name();
    case InstanceRole: {
        QVariant var;
        var.setValue(instance);
        return var;
    }
    case InstanceIdentifierRole:
        return instance.identifier();
    case Qt::ToolTipRole:
        return QStringLiteral("<qt><h4>%1</h4>%2</qt>").arg(instance.name(), instance.type().description());
    case StatusRole:
        return instance.status();
    case StatusMessageRole:
        return instance.statusMessage();
    case ProgressRole:
        return instance.progress();
    case OnlineRole:
        return instance.isOnline();
    case TypeRole: {
        QVariant var;
        var.setValue(instance.type());
        return var;
    }
    case TypeIdentifierRole:
        return instance.type().identifier();
    case DescriptionRole:
        return instance.type().description();
    case CapabilitiesRole:
        return instance.type().capabilities();
    case MimeTypesRole:
        return instance.type().mimeTypes();
    }
    return QVariant();
}

QVariant AgentInstanceModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical) {
        return QVariant();
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (section) {
    case 0:
        return i18nc("@title:column, name of a thing", "Name");
    default:
        return QVariant();
    }
}

QModelIndex AgentInstanceModel::index(int row, int column, const QModelIndex & /*parent*/) const
{
    if (row < 0 || row >= d->mInstances.count()) {
        return QModelIndex();
    }

    if (column != 0) {
        return QModelIndex();
    }

    return createIndex(row, column);
}

QModelIndex AgentInstanceModel::parent(const QModelIndex & /*child*/) const
{
    return QModelIndex();
}

Qt::ItemFlags AgentInstanceModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= d->mInstances.count()) {
        return QAbstractItemModel::flags(index);
    }

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool AgentInstanceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    if (index.row() < 0 || index.row() >= d->mInstances.count()) {
        return false;
    }

    AgentInstance &instance = d->mInstances[index.row()];

    switch (role) {
    case OnlineRole:
        instance.setIsOnline(value.toBool());
        Q_EMIT dataChanged(index, index);
        return true;
    default:
        return false;
    }

    return false;
}

#include "moc_agentinstancemodel.cpp"
