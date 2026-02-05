/*
    SPDX-FileCopyrightText: 2006-2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

#include <QAbstractItemModel>

#include <memory>

namespace Akonadi
{
class AgentInstanceModelPrivate;

/**
 * @short Provides a data model for agent instances.
 *
 * This class provides the interface of a QAbstractItemModel to
 * access all available agent instances: their name, identifier,
 * supported mimetypes and capabilities.
 *
 * @code
 *
 * Akonadi::AgentInstanceModel *model = new Akonadi::AgentInstanceModel( this );
 *
 * QListView *view = new QListView( this );
 * view->setModel( model );
 *
 * @endcode
 *
 * To show only agent instances that match a given mime type or special
 * capabilities, use the AgentFilterProxyModel on top of this model.
 *
 * \author Tobias Koenig <tokoe@kde.org>
 */
class AKONADICORE_EXPORT AgentInstanceModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    /**
     * Describes the roles of this model.
     */
    enum Roles {
        TypeRole = Qt::UserRole + 1, ///< The agent type itself
        NameRole, /// The display name of the agent type
        IconNameRole, /// The icon name of the agent
        TypeIdentifierRole, ///< The identifier of the agent type
        DescriptionRole, ///< A description of the agent type
        MimeTypesRole, ///< A list of supported mimetypes
        CapabilitiesRole, ///< A list of supported capabilities
        InstanceRole, ///< The agent instance itself
        InstanceIdentifierRole, ///< The identifier of the agent instance
        StatusRole, ///< The current status (numerical) of the instance
        StatusMessageRole, ///< A textual presentation of the current status
        ProgressRole, ///< The current progress (numerical in percent) of an operation
        OnlineRole, ///< The current online/offline status
        ActivitiesRole, ///< A list of activities
        ActivitiesEnabledRole, ///< Return true if activities is enabled on agent
        UserRole = Qt::UserRole + 42 ///< Role for user extensions
    };

    /**
     * Creates a new agent instance model.
     *
     * @param parent The parent object.
     */
    explicit AgentInstanceModel(QObject *parent = nullptr);

    /**
     * Destroys the agent instance model.
     */
    ~AgentInstanceModel() override;

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QModelIndex parent(const QModelIndex &index) const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
    [[nodiscard]] bool setData(const QModelIndex &index, const QVariant &value, int role) override;

private:
    /// @cond PRIVATE
    friend class AgentInstanceModelPrivate;
    std::unique_ptr<AgentInstanceModelPrivate> const d;
    /// @endcond
};

}
