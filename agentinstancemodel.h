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
 * @short The AgentInstanceModel provides a data model for Akonadi Agent Instances.
 *
 * This class provides access to the Agent Instances of Akonadi, their name, identifier,
 * supported mimetypes and capabilities.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADI_EXPORT AgentInstanceModel : public QAbstractItemModel
{
  Q_OBJECT

  public:
    // keep in sync with the agenttypemodel.h
    /**
     * Describes the roles of this model.
     */
    enum Role
    {
      TypeIdentifierRole  = Qt::UserRole + 1,  ///< The identifier of the agent type
      CommentRole,                             ///< A description of the agent type
      MimeTypesRole,                           ///< A list of supported mimetypes
      CapabilitiesRole,                        ///< A list of supported capabilities
      InstanceIdentifierRole,                  ///< The identifier of the agent instance
      StatusRole,                              ///< The current status (numerical) of the instance
      StatusMessageRole,                       ///< A textual presentation of the current status
      ProgressRole,                            ///< The current progress (numerical in percent) of an operation
      ProgressMessageRole,                     ///< A textual presentation of the current progress
      OnlineRole                               ///< The current online/offline status
    };

    /**
     * Creates a new agent instance model.
     */
    explicit AgentInstanceModel( QObject *parent );

    /**
     * Destroys the agent instance model.
     */
    virtual ~AgentInstanceModel();

    /**
     * Reimplemented from QAbstractItemModel.
     */
    virtual int columnCount( const QModelIndex &parent = QModelIndex() ) const;

    /**
     * Reimplemented from QAbstractItemModel.
     */
    virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const;

    /**
     * Reimplemented from QAbstractItemModel.
     */
    virtual QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;

    /**
     * Reimplemented from QAbstractItemModel.
     */
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    /**
     * Reimplemented from QAbstractItemModel.
     */
    virtual QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const;

    /**
     * Reimplemented from QAbstractItemModel.
     */
    virtual QModelIndex parent( const QModelIndex &index ) const;

    /**
     * Reimplemented from QAbstractItemModel.
     */
    virtual Qt::ItemFlags flags( const QModelIndex &index ) const;

    /**
     * Reimplemented from QAbstractItemModel.
     */
    virtual bool setData( const QModelIndex &index, const QVariant &value, int role );

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void agentInstanceAdded( const QString& ) )
    Q_PRIVATE_SLOT( d, void agentInstanceRemoved( const QString& ) )
    Q_PRIVATE_SLOT( d, void agentInstanceStatusChanged( const QString&, AgentManager::Status, const QString& ) )
    Q_PRIVATE_SLOT( d, void agentInstanceProgressChanged( const QString&, uint, const QString& ) )
    Q_PRIVATE_SLOT( d, void agentInstanceNameChanged( const QString&, const QString& ) )
    Q_PRIVATE_SLOT( d, void addAgentInstance( const QString &agentInstance ) )
};

}

#endif
