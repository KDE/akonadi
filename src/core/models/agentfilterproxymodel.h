/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include <QSortFilterProxyModel>

#include <memory>

namespace Akonadi
{
class AgentFilterProxyModelPrivate;
/*!
 * \brief A proxy model for filtering AgentType or AgentInstance
 *
 * This filter proxy model works on top of a AgentTypeModel or AgentInstanceModel
 * and can be used to show only AgentType or AgentInstance objects
 * which provide a given mime type or capability.
 *
 * \code
 *
 * // Show only running agent instances that provide contacts
 * Akonadi::AgentInstanceModel *model = new Akonadi::AgentInstanceModel( this );
 *
 * Akonadi::AgentFilterProxyModel *proxy = new Akonadi::AgentFilterProxyModel( this );
 * proxy->addMimeTypeFilter( "text/directory" );
 *
 * proxy->setSourceModel( model );
 *
 * QListView *view = new QListView( this );
 * view->setModel( proxy );
 *
 * \endcode
 *
 * \author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT AgentFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    /*!
     * Create a new agent filter proxy model.
     * By default no filtering is done.
     * \a parent parent object
     */
    explicit AgentFilterProxyModel(QObject *parent = nullptr);

    /*!
     * Destroys the agent filter proxy model.
     */
    ~AgentFilterProxyModel() override;

    /*!
     * Accept agents supporting \a mimeType.
     */
    void addMimeTypeFilter(const QString &mimeType);

    /*!
     * Accept agents with the given \a capability.
     */
    void addCapabilityFilter(const QString &capability);

    /*!
     * Clear the filters ( mimeTypes & capabilities ).
     */
    void clearFilters();

    /*!
     * Excludes agents with the given \a capability.
     * \a capability undesired agent capability
     * \since 4.6
     */
    void excludeCapabilities(const QString &capability);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;

private:
    std::unique_ptr<AgentFilterProxyModelPrivate> const d;
};

}
