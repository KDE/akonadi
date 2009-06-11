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

#ifndef AKONADI_AGENTTYPEMODEL_H
#define AKONADI_AGENTTYPEMODEL_H

#include "akonadi_export.h"

#include <QtCore/QAbstractItemModel>

namespace Akonadi {

/**
 * @short Provides a data model for agent types.
 *
 * This class provides the interface of a QAbstractItemModel to
 * access all available agent types: their name, identifier,
 * supported mimetypes and capabilities.
 *
 * @code
 *
 * Akonadi::AgentTypeModel *model = new Akonadi::AgentTypeModel( this );
 *
 * QListView *view = new QListView( this );
 * view->setModel( model );
 *
 * @endcode
 *
 * To show only agent types that match a given mime type or special
 * capabilities, use the AgentFilterProxyModel on top of this model.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADI_EXPORT AgentTypeModel : public QAbstractItemModel
{
  Q_OBJECT

  public:
    /**
     * Describes the roles of this model.
     */
    enum Roles
    {
      TypeRole = Qt::UserRole + 1,            ///< The agent type itself
      IdentifierRole,                         ///< The identifier of the agent type
      DescriptionRole,                        ///< A description of the agent type
      MimeTypesRole,                          ///< A list of supported mimetypes
      CapabilitiesRole,                       ///< A list of supported capabilities
      UserRole  = Qt::UserRole + 42           ///< Role for user extensions
    };

    /**
     * Creates a new agent type model.
     */
    explicit AgentTypeModel( QObject *parent = 0 );

    /**
     * Destroys the agent type model.
     */
    virtual ~AgentTypeModel();

    virtual int columnCount( const QModelIndex &parent = QModelIndex() ) const;
    virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const;
    virtual QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;
    virtual QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const;
    virtual QModelIndex parent( const QModelIndex &index ) const;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const;

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void typeAdded( const Akonadi::AgentType& ) )
    Q_PRIVATE_SLOT( d, void typeRemoved( const Akonadi::AgentType& ) )
    //@endcond
};

}

#endif
