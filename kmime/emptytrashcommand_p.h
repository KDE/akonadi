/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Copyright (c) 2010 Andras Mantia <andras@kdab.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef EMPTYTRASHCOMMAND_P_H
#define EMPTYTRASHCOMMAND_P_H

#include <commandbase_p.h>

#include "akonadi/agentinstance.h"
#include "akonadi/collection.h"

class QAbstractItemModel;
class KJob;

class EmptyTrashCommand : public CommandBase
{
  Q_OBJECT

public:
    EmptyTrashCommand(const QAbstractItemModel* model, QObject* parent);
    EmptyTrashCommand(const Akonadi::Collection& folder, QObject* parent);
    /*reimp*/ void execute();

private slots:
  void slotExpungeJob( KJob *job );
  void slotDeleteJob( KJob *job );

protected Q_SLOTS:
    void emitResult( Result result );

private:
  void expunge( const Akonadi::Collection& );
  Akonadi::AgentInstance::List agentInstances();
  Akonadi::Collection trashCollectionFolder();
  Akonadi::Collection collectionFromId(const Akonadi::Collection::Id& id) const;
  bool folderIsTrash( const Akonadi::Collection & col );

  const QAbstractItemModel* mModel;
  Akonadi::Collection::Id the_trashCollectionFolder;
  Akonadi::Collection mFolder;
  int mNumberOfTrashToEmpty;
};

#endif // EMPTYTRASHCOMMAND_P_H
