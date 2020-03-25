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

#ifndef AKONADI_TAGMODEL_H
#define AKONADI_TAGMODEL_H

#include <QAbstractItemModel>

#include "akonadicore_export.h"
#include "tag.h"

class KJob;

namespace Akonadi
{

class Monitor;
class TagModelPrivate;

class AKONADICORE_EXPORT TagModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        TypeRole,
        GIDRole,
        ParentRole,
        TagRole,

        UserRole = Qt::UserRole + 500,
        TerminalUserRole = 2000,
        EndRole = 65535
    };

    explicit TagModel(Monitor *recorder, QObject *parent = nullptr);
    ~TagModel() override;

    Q_REQUIRED_RESULT int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    Q_REQUIRED_RESULT int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    Q_REQUIRED_RESULT QVariant data(const QModelIndex &index, int role) const override;
    Q_REQUIRED_RESULT QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Q_REQUIRED_RESULT Qt::ItemFlags flags(const QModelIndex &index) const override;
    /*
    virtual Qt::DropActions supportedDropActions() const;
    virtual QMimeData* mimeData( const QModelIndexList &indexes ) const;
    virtual bool dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent );
    */

    Q_REQUIRED_RESULT QModelIndex parent(const QModelIndex &child) const override;
    Q_REQUIRED_RESULT QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

protected:
    Q_DECLARE_PRIVATE(TagModel)
    TagModelPrivate *d_ptr;

    TagModel(Monitor *recorder, TagModelPrivate *dd, QObject *parent);

Q_SIGNALS:
    void populated();

private:
    bool insertRows(int row, int count, const QModelIndex &index = QModelIndex()) override;
    bool insertColumns(int column, int count, const QModelIndex &index = QModelIndex()) override;
    bool removeColumns(int column, int count, const QModelIndex &index = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &index = QModelIndex()) override;

    Q_PRIVATE_SLOT(d_func(), void fillModel())
    Q_PRIVATE_SLOT(d_func(), void tagsFetched(const Akonadi::Tag::List &tags))
    Q_PRIVATE_SLOT(d_func(), void tagsFetchDone(KJob *job))
    Q_PRIVATE_SLOT(d_func(), void monitoredTagAdded(const Akonadi::Tag &tag))
    Q_PRIVATE_SLOT(d_func(), void monitoredTagRemoved(const Akonadi::Tag &tag))
    Q_PRIVATE_SLOT(d_func(), void monitoredTagChanged(const Akonadi::Tag &tag))
};
}

#endif // AKONADI_TAGMODEL_H
