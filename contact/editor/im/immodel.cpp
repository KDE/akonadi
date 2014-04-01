/*
    This file is part of Akonadi Contact.

    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "immodel.h"

#include "improtocols.h"

#include <kicon.h>
#include <klocalizedstring.h>

IMAddress::IMAddress()
    : mProtocol(QStringLiteral("messaging/aim"))
    , mPreferred(false)
{
}

IMAddress::IMAddress(const QString &protocol, const QString &name, bool preferred)
    : mProtocol(protocol)
    , mName(name)
    , mPreferred(preferred)
{
}

void IMAddress::setProtocol(const QString &protocol)
{
    mProtocol = protocol;
}

QString IMAddress::protocol() const
{
    return mProtocol;
}

void IMAddress::setName(const QString &name)
{
    mName = name;
}

QString IMAddress::name() const
{
    return mName;
}

void IMAddress::setPreferred(bool preferred)
{
    mPreferred = preferred;
}

bool IMAddress::preferred() const
{
    return mPreferred;
}

IMModel::IMModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

IMModel::~IMModel()
{
}

void IMModel::setAddresses(const IMAddress::List &addresses)
{
    emit layoutAboutToBeChanged();

    mAddresses = addresses;

    emit layoutChanged();
}

IMAddress::List IMModel::addresses() const
{
    return mAddresses;
}

QModelIndex IMModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return createIndex(row, column);
}

QModelIndex IMModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);
    return QModelIndex();
}

QVariant IMModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() < 0 || index.row() >= mAddresses.count()) {
        return QVariant();
    }

    if (index.column() < 0 || index.column() > 1) {
        return QVariant();
    }

    const IMAddress &address = mAddresses[index.row()];

    if (role == Qt::DisplayRole) {
        if (index.column() == 0) {
            return IMProtocols::self()->name(address.protocol());
        } else {
            return address.name();
        }
    }

    if (role == Qt::DecorationRole) {
        if (index.column() == 1) {
            return QVariant();
        }

        return QIcon::fromTheme(IMProtocols::self()->icon(address.protocol()));
    }

    if (role == Qt::EditRole) {
        if (index.column() == 0) {
            return address.protocol();
        } else {
            return address.name();
        }
    }

    if (role == ProtocolRole) {
        return address.protocol();
    }

    if (role == IsPreferredRole) {
        return address.preferred();
    }

    return QVariant();
}

bool IMModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    if (index.row() < 0 || index.row() >= mAddresses.count()) {
        return false;
    }

    if (index.column() < 0 || index.column() > 1) {
        return false;
    }

    IMAddress &address = mAddresses[index.row()];

    if (role == Qt::EditRole) {
        if (index.column() == 1) {
            address.setName(value.toString());
            emit dataChanged(index, index);
            return true;
        }
    }

    if (role == ProtocolRole) {
        address.setProtocol(value.toString());
        emit dataChanged(this->index(index.row(), 0), this->index(index.row(), 1));
        return true;
    }

    if (role == IsPreferredRole) {
        address.setPreferred(value.toBool());
        emit dataChanged(this->index(index.row(), 0), this->index(index.row(), 1));
        return true;
    }

    return false;
}

QVariant IMModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section < 0 || section > 1) {
        return QVariant();
    }

    if (orientation != Qt::Horizontal) {
        return QVariant();
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (section == 0) {
        return i18nc("instant messaging protocol", "Protocol");
    } else {
        return i18nc("instant messaging address", "Address");
    }
}

Qt::ItemFlags IMModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= mAddresses.count()) {
        return QAbstractItemModel::flags(index);
    }

    const Qt::ItemFlags parentFlags = QAbstractItemModel::flags(index);
    return (parentFlags | Qt::ItemIsEnabled | Qt::ItemIsEditable);
}

int IMModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return 2;
    } else {
        return 0;
    }
}

int IMModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return mAddresses.count();
    } else {
        return 0;
    }
}

bool IMModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid()) {
        return false;
    }

    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        mAddresses.insert(row, IMAddress());
    }
    endInsertRows();

    return true;
}

bool IMModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid()) {
        return false;
    }

    beginRemoveRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        mAddresses.remove(row);
    }
    endRemoveRows();

    return true;
}
