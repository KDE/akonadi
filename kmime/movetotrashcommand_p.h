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

#ifndef MOVETOTRASHCOMMAND_H
#define MOVETOTRASHCOMMAND_H

#include "commandbase_p.h"

#include <akonadi/collection.h>
#include <akonadi/item.h>

#include <QList>

class QAbstractItemModel;
class KJob;
class MoveToTrashCommand : public CommandBase
{
  Q_OBJECT
public:
  MoveToTrashCommand( const QAbstractItemModel* model, const QList< Akonadi::Item >& msgList, QObject* parent = 0 );
  MoveToTrashCommand( const QAbstractItemModel* model, const Akonadi::Collection::List& folders, QObject* parent = 0 );

  /*reimp*/ void execute();

private Q_SLOTS:
  void slotFetchDone( KJob* job );
  void slotMoveDone( const Result &result);

private:
  void moveMessages();
  Akonadi::Collection trashCollectionFromResource( const Akonadi::Collection & col );
  Akonadi::Collection trashCollectionFolder();
  Akonadi::Collection findTrashFolder( const Akonadi::Collection& folder );
  Akonadi::Collection collectionFromId(const Akonadi::Collection::Id& id) const;

  Akonadi::Collection::List mFolders;
  QList<Akonadi::Item> mMessages;
  Akonadi::Collection::Id the_trashCollectionFolder;
  const QAbstractItemModel* mModel;
  int mFolderListJobCount;
};

#endif // MOVETOTRASHCOMMAND_H
