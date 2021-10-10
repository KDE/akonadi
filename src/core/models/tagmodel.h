/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QAbstractItemModel>

#include "akonadicore_export.h"
#include "tag.h"

#include <memory>

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
    std::unique_ptr<TagModelPrivate> const d_ptr;

    TagModel(Monitor *recorder, TagModelPrivate *dd, QObject *parent);

Q_SIGNALS:
    void populated();

private:
    bool insertRows(int row, int count, const QModelIndex &index = QModelIndex()) override;
    bool insertColumns(int column, int count, const QModelIndex &index = QModelIndex()) override;
    bool removeColumns(int column, int count, const QModelIndex &index = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &index = QModelIndex()) override;
    // Used by FakeAkonadiServerCommand::connectForwardingSignals (tagmodeltest)
    Q_PRIVATE_SLOT(d_func(), void tagsFetched(const Akonadi::Tag::List &tags))
    Q_PRIVATE_SLOT(d_func(), void monitoredTagAdded(const Akonadi::Tag &tag))
    Q_PRIVATE_SLOT(d_func(), void monitoredTagRemoved(const Akonadi::Tag &tag))
    Q_PRIVATE_SLOT(d_func(), void monitoredTagChanged(const Akonadi::Tag &tag))
};
}

