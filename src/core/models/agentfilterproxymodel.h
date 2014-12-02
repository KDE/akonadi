/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_AGENTFILTERPROXYMODEL_H
#define AKONADI_AGENTFILTERPROXYMODEL_H

#include "akonadicore_export.h"
#include <QSortFilterProxyModel>

namespace Akonadi {

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
    explicit AgentFilterProxyModel(QObject *parent = Q_NULLPTR);

    /**
     * Destroys the agent filter proxy model.
     */
    ~AgentFilterProxyModel();

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
    bool filterAcceptsRow(int row, const QModelIndex &parent) const Q_DECL_OVERRIDE;

private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

}

#endif
