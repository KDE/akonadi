/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_AGENTFILTERPROXYMODEL_H
#define AKONADI_AGENTFILTERPROXYMODEL_H

#include "akonadicore_export.h"
#include <QSortFilterProxyModel>

namespace Akonadi
{

/**
 * @short A proxy model for filtering AgentType or AgentInstance
 *
 * This filter proxy model works on top of a AgentTypeModel or AgentInstanceModel
 * and can be used to show only AgentType or AgentInstance objects
 * which provide a given mime type or capability.
 *
 * @code
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
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT AgentFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    /**
     * Create a new agent filter proxy model.
     * By default no filtering is done.
     * @param parent parent object
     */
    explicit AgentFilterProxyModel(QObject *parent = nullptr);

    /**
     * Destroys the agent filter proxy model.
     */
    ~AgentFilterProxyModel() override;

    /**
     * Accept agents supporting @p mimeType.
     */
    void addMimeTypeFilter(const QString &mimeType);

    /**
     * Accept agents with the given @p capability.
     */
    void addCapabilityFilter(const QString &capability);

    /**
     * Clear the filters ( mimeTypes & capabilities ).
     */
    void clearFilters();

    /**
     * Excludes agents with the given @p capability.
     * @param capability undesired agent capability
     * @since 4.6
     */
    void excludeCapabilities(const QString &capability);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;

private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

}

#endif
