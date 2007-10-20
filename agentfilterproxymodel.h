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

#include "libakonadi_export.h"
#include <QSortFilterProxyModel>

namespace Akonadi {

/**
  Filter agent type or agent instance models for supported mime types
  and/or agent capabilities.
*/
class AKONADI_EXPORT AgentFilterProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
  public:
    /**
      Create a new agent filter proxy model.
      Shows all agents by default.
    */
    AgentFilterProxyModel( QObject *parent = 0 );
    ~AgentFilterProxyModel();

    /**
      Accept agents supporting @p mimeType.
    */
    void addMimeType( const QString &mimeType );

    /**
      Accept agents with the given capability.
    */
    void addCapability( const QString &capability );

    /**
      Clear the filters.
    */
    void clearFilter();

  protected:
    bool filterAcceptsRow( int row, const QModelIndex &parent ) const;

  private:
    class Private;
    Private* const d;
};

}

#endif
