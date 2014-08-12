/*
    Copyright (c) 2006 - 2008 Volker Krause <vkrause@kde.org>

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

#include "collectionmodel.h"
#include "collectionmodel_p.h"

#include "collectionutils.h"
#include "collectionmodifyjob.h"
#include "entitydisplayattribute.h"
#include "monitor.h"
#include "pastehelper_p.h"
#include "session.h"

#include <qdebug.h>
#include <QIcon>
#include <QUrl>


#include <QtCore/QMimeData>

using namespace Akonadi;

CollectionModel::CollectionModel(QObject *parent)
    : QAbstractItemModel(parent)
    , d_ptr(new CollectionModelPrivate(this))
{
    Q_D(CollectionModel);
    d->init();
}

//@cond PRIVATE
CollectionModel::CollectionModel(CollectionModelPrivate *d, QObject *parent)
    : QAbstractItemModel(parent)
    , d_ptr(d)
{
    d->init();
}
//@endcond

CollectionModel::~CollectionModel()
{
    Q_D(CollectionModel);
    d->childCollections.clear();
    d->collections.clear();

    delete d->monitor;
    d->monitor = 0;

    delete d;
}

int CollectionModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0) {
        return 0;
    }
    return 1;
}

QVariant CollectionModel::data(const QModelIndex &index, int role) const
{
    Q_D(const CollectionModel);
    if (!index.isValid()) {
        return QVariant();
    }

    const Collection col = d->collections.value(index.internalId());
    if (!col.isValid()) {
        return QVariant();
    }

    if (index.column() == 0 && (role == Qt::DisplayRole || role == Qt::EditRole)) {
        return col.displayName();
    }

    switch (role) {
    case Qt::DecorationRole:
        if (index.column() == 0) {
            if (col.hasAttribute<EntityDisplayAttribute>() &&
                !col.attribute<EntityDisplayAttribute>()->iconName().isEmpty()) {
                return col.attribute<EntityDisplayAttribute>()->icon();
            }
            return QIcon::fromTheme(CollectionUtils::defaultIconName(col));
        }
        break;
    case OldCollectionIdRole: // fall-through
    case CollectionIdRole:
        return col.id();
    case OldCollectionRole: // fall-through
    case CollectionRole:
        return QVariant::fromValue(col);
    }
    return QVariant();
}

QModelIndex CollectionModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_D(const CollectionModel);
    if (column >= columnCount() || column < 0) {
        return QModelIndex();
    }

    QVector<Collection::Id> list;
    if (!parent.isValid()) {
        list = d->childCollections.value(Collection::root().id());
    } else {
        if (parent.column() > 0) {
            return QModelIndex();
        }
        list = d->childCollections.value(parent.internalId());
    }

    if (row < 0 || row >= list.size()) {
        return QModelIndex();
    }
    if (!d->collections.contains(list.at(row))) {
        return QModelIndex();
    }
    return createIndex(row, column, reinterpret_cast<void *>(d->collections.value(list.at(row)).id()));
}

QModelIndex CollectionModel::parent(const QModelIndex &index) const
{
    Q_D(const CollectionModel);
    if (!index.isValid()) {
        return QModelIndex();
    }

    const Collection col = d->collections.value(index.internalId());
    if (!col.isValid()) {
        return QModelIndex();
    }

    const Collection parentCol = d->collections.value(col.parentCollection().id());
    if (!parentCol.isValid()) {
        return QModelIndex();
    }
    QVector<Collection::Id> list;
    list = d->childCollections.value(parentCol.parentCollection().id());

    int parentRow = list.indexOf(parentCol.id());
    if (parentRow < 0) {
        return QModelIndex();
    }

    return createIndex(parentRow, 0, reinterpret_cast<void *>(parentCol.id()));
}

int CollectionModel::rowCount(const QModelIndex &parent) const
{
    const  Q_D(CollectionModel);
    QVector<Collection::Id> list;
    if (parent.isValid()) {
        list = d->childCollections.value(parent.internalId());
    } else {
        list = d->childCollections.value(Collection::root().id());
    }

    return list.size();
}

QVariant CollectionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    const  Q_D(CollectionModel);

    if (section == 0 && orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return d->headerContent;
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

bool CollectionModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    Q_D(CollectionModel);

    if (section == 0 && orientation == Qt::Horizontal && role == Qt::EditRole) {
        d->headerContent = value.toString();
        return true;
    }

    return false;
}

bool CollectionModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_D(CollectionModel);
    if (index.column() == 0 && role == Qt::EditRole) {
        // rename collection
        Collection col = d->collections.value(index.internalId());
        if (!col.isValid() || value.toString().isEmpty()) {
            return false;
        }
        col.setName(value.toString());
        CollectionModifyJob *job = new CollectionModifyJob(col, d->session);
        connect(job, SIGNAL(result(KJob*)), SLOT(editDone(KJob*)));
        return true;
    }
    return QAbstractItemModel::setData(index, value, role);
}

Qt::ItemFlags CollectionModel::flags(const QModelIndex &index) const
{
    Q_D(const CollectionModel);

    // Pass modeltest.
    if (!index.isValid()) {
        return 0;
    }

    Qt::ItemFlags flags = QAbstractItemModel::flags(index);

    flags = flags | Qt::ItemIsDragEnabled;

    Collection col;
    if (index.isValid()) {
        col = d->collections.value(index.internalId());
        Q_ASSERT(col.isValid());
    } else {
        return flags | Qt::ItemIsDropEnabled; // HACK Workaround for a probable bug in Qt
    }

    if (col.isValid()) {
        if (col.rights() & (Collection::CanChangeCollection |
                            Collection::CanCreateCollection |
                            Collection::CanDeleteCollection |
                            Collection::CanCreateItem))  {
            if (index.column() == 0) {
                flags = flags | Qt::ItemIsEditable;
            }
            flags = flags | Qt::ItemIsDropEnabled;
        }
    }

    return flags;
}

Qt::DropActions CollectionModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList CollectionModel::mimeTypes() const
{
    return QStringList() << QStringLiteral("text/uri-list");
}

QMimeData *CollectionModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *data = new QMimeData();
    QList<QUrl> urls;
    foreach (const QModelIndex &index, indexes) {
        if (index.column() != 0) {
            continue;
        }

        urls << Collection(index.internalId()).url();
    }
    data->setUrls(urls);

    return data;
}

bool CollectionModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_D(CollectionModel);
    if (!(action & supportedDropActions())) {
        return false;
    }

    // handle drops onto items as well as drops between items
    QModelIndex idx;
    if (row >= 0 && column >= 0) {
        idx = index(row, column, parent);
    } else {
        idx = parent;
    }

    if (!idx.isValid()) {
        return false;
    }

    const Collection parentCol = d->collections.value(idx.internalId());
    if (!parentCol.isValid()) {
        return false;
    }

    KJob *job = PasteHelper::paste(data, parentCol, action != Qt::MoveAction);
    connect(job, SIGNAL(result(KJob*)), SLOT(dropResult(KJob*)));
    return true;
}

Collection CollectionModel::collectionForId(Collection::Id id) const
{
    Q_D(const CollectionModel);
    return d->collections.value(id);
}

void CollectionModel::fetchCollectionStatistics(bool enable)
{
    Q_D(CollectionModel);
    d->fetchStatistics = enable;
    d->monitor->fetchCollectionStatistics(enable);
}

void CollectionModel::includeUnsubscribed(bool include)
{
    Q_D(CollectionModel);
    d->unsubscribed = include;
}

#include "moc_collectionmodel.cpp"
