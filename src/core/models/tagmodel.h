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

/*!
 * \class Akonadi::TagModel
 * \inheaderfile Akonadi/TagModel
 * \inmodule AkonadiCore
 */
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

    //! Constructs a TagModel object.
    //! @param recorder The Monitor instance to watch for tag changes.
    //! @param parent The parent object, passed to QAbstractItemModel.
    explicit TagModel(Monitor *recorder, QObject *parent = nullptr);
    //! Destructor.
    ~TagModel() override;

    //! Returns the number of columns for the given model index.
    //! @param parent The parent index.
    //! @return The number of columns, which is 1 for the tag model.
    [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    //! Returns the number of rows under the given parent index.
    //! @param parent The parent index.
    //! @return The number of child rows.
    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    //! Returns the data stored under the given role for the item at the given index.
    //! @param index The model index.
    //! @param role The data role.
    //! @return The data for the given role.
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    //! Returns the header data for the given section, orientation, and role.
    //! @param section The column section.
    //! @param orientation The header orientation.
    //! @param role The data role.
    //! @return The header data.
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    //! Returns the flags for the item at the given index.
    //! @param index The model index.
    //! @return The item flags.
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
    /*
    virtual Qt::DropActions supportedDropActions() const;
    virtual QMimeData* mimeData( const QModelIndexList &indexes ) const;
    virtual bool dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent );
    */

    //! Returns the parent index of the given child index.
    //! @param child The child index.
    //! @return The parent index.
    [[nodiscard]] QModelIndex parent(const QModelIndex &child) const override;
    //! Returns the index of the item in the model specified by the given row, column, and parent index.
    //! @param row The row number.
    //! @param column The column number.
    //! @param parent The parent index.
    //! @return The model index at the specified position.
    [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

protected:
    Q_DECLARE_PRIVATE(TagModel)
    std::unique_ptr<TagModelPrivate> const d_ptr;

    //! Constructs a TagModel object with a private implementation.
    //! @param recorder The Monitor instance to watch for tag changes.
    //! @param dd The private implementation object.
    //! @param parent The parent object.
    TagModel(Monitor *recorder, TagModelPrivate *dd, QObject *parent);

Q_SIGNALS:
    //! Emitted when the tag model has been fully populated with data from the Monitor.
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
