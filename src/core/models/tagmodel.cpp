/*
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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

#include "tagmodel.h"
#include "tagmodel_p.h"
#include "tagattribute.h"

#include <KLocalizedString>
#include <QIcon>

using namespace Akonadi;

TagModel::TagModel(Monitor *recorder, QObject *parent)
    : QAbstractItemModel(parent)
    , d_ptr(new TagModelPrivate(this))
{
    Q_D(TagModel);
    d->init(recorder);
}

TagModel::TagModel(Monitor *recorder, TagModelPrivate *dd, QObject *parent)
    : QAbstractItemModel(parent)
    , d_ptr(dd)
{
    Q_D(TagModel);
    d->init(recorder);
}

TagModel::~TagModel()
{
    delete d_ptr;
}

int TagModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0) {
        return 0;
    }

    return 1;
}

int TagModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const TagModel);

    Tag::Id parentTagId = -1;
    if (parent.isValid()) {
        parentTagId = d->mChildTags[parent.internalId()].at(parent.row()).id();
    }

    return d->mChildTags[parentTagId].count();
}

QVariant TagModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        switch (section) {
        case 0:
            return i18n("Tag");
        }
    }

    return QAbstractItemModel::headerData(section, orientation, role);
}

QVariant TagModel::data(const QModelIndex &index, int role) const
{
    Q_D(const TagModel);

    if (!index.isValid()) {
        return QVariant();
    }
    const Tag tag = d->tagForIndex(index);
    if (!tag.isValid()) {
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole: // fall-through
    case NameRole:
        return tag.name();
    case IdRole:
        return tag.id();
    case GIDRole:
        return tag.gid();
    case ParentRole:
        return QVariant::fromValue(tag.parent());
    case TagRole:
        return QVariant::fromValue(tag);
    case Qt::DecorationRole: {
        if (const TagAttribute *attr = tag.attribute<TagAttribute>()) {
            return QIcon::fromTheme(attr->iconName());
        } else {
            return QVariant();
        }
    }
    }

    return QVariant();
}

QModelIndex TagModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_D(const TagModel);

    qint64 parentId = -1;
    if (parent.isValid()) {
        const Tag parentTag = d->tagForIndex(parent);
        parentId = parentTag.id();
    }

    const Tag::List &children = d->mChildTags.value(parentId);
    if (row >= children.count()) {
        return QModelIndex();
    }

    return createIndex(row, column, (int) parentId);
}

QModelIndex TagModel::parent(const QModelIndex &child) const
{
    Q_D(const TagModel);

    if (!child.isValid()) {
        return QModelIndex();
    }

    const qint64 parentId = child.internalId();
    return d->indexForTag(parentId);
}

Qt::ItemFlags TagModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
}

bool TagModel::insertColumns(int, int, const QModelIndex &)
{
    return false;
}

bool TagModel::insertRows(int, int, const QModelIndex &)
{
    return false;
}

bool TagModel::removeColumns(int, int, const QModelIndex &)
{
    return false;
}

bool TagModel::removeRows(int, int, const QModelIndex &)
{
    return false;
}

#include "moc_tagmodel.cpp"
