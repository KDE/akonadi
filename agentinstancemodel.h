/*
    Copyright (c) 2006-2008 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_AGENTINSTANCEMODEL_H
#define AKONADI_AGENTINSTANCEMODEL_H

#include "akonadi_export.h"

#include <QtCore/QAbstractItemModel>

namespace Akonadi {

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
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADI_EXPORT AgentInstanceModel : public QAbstractItemModel
{
  Q_OBJECT

  public:
    /**
     * Describes the roles of this model.
     */
    enum Roles
    {
      TypeRole = Qt::UserRole + 1,             ///< The agent type itself
      TypeIdentifierRole,                      ///< The identifier of the agent type
      DescriptionRole,                         ///< A description of the agent type
      MimeTypesRole,                           ///< A list of supported mimetypes
      CapabilitiesRole,                        ///< A list of supported capabilities
      InstanceRole,                            ///< The agent instance itself
      InstanceIdentifierRole,                  ///< The identifier of the agent instance
      StatusRole,                              ///< The current status (numerical) of the instance
      StatusMessageRole,                       ///< A textual presentation of the current status
      ProgressRole,                            ///< The current progress (numerical in percent) of an operation
      OnlineRole,                              ///< The current online/offline status
      UserRole  = Qt::UserRole + 42            ///< Role for user extensions
    };

    /**
     * Creates a new agent instance model.
     *
     * @param parent The parent object.
     */
    explicit AgentInstanceModel( QObject *parent = 0 );

    /**
     * Destroys the agent instance model.
     */
    virtual ~AgentInstanceModel();

    virtual int columnCount( const QModelIndex &parent = QModelIndex() ) const;
    virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const;
    virtual QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    virtual QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const;
    virtual QModelIndex parent( const QModelIndex &index ) const;
    virtual Qt::ItemFlags flags( const QModelIndex &index ) const;
    virtual bool setData( const QModelIndex &index, const QVariant &value, int role );

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void instanceAdded( const Akonadi::AgentInstance& ) )
    Q_PRIVATE_SLOT( d, void instanceRemoved( const Akonadi::AgentInstance& ) )
    Q_PRIVATE_SLOT( d, void instanceChanged( const Akonadi::AgentInstance& ) )
    //@endcond
};

}

#endif
