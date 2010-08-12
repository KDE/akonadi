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


#include "movetotrashcommand.h"
#include "util.h"
#include "movecommand.h"
#include "imapsettings.h"

#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>

#include <Akonadi/KMime/SpecialMailCollections>
#include <Akonadi/EntityTreeModel>

#define IMAP_RESOURCE_IDENTIFIER QString::fromLatin1("akonadi_imap_resource")
MoveToTrashCommand::MoveToTrashCommand(QAbstractItemModel* model, const Akonadi::Collection& sourceFolder, QObject* parent): CommandBase( parent )
{
  the_trashCollectionFolder = -1;
  mSourceFolder = sourceFolder;
  mModel = model;
}

MoveToTrashCommand::MoveToTrashCommand(QAbstractItemModel* model, const QList< Akonadi::Item >& msgList, QObject* parent): CommandBase( parent )
{
  the_trashCollectionFolder = -1;
  mMessages = msgList;
  mModel = model;
}


void MoveToTrashCommand::slotFetchDone(KJob* job)
{
  if ( job->error() ) {
    // handle errors
    Util::showJobError(job);
    emitResult( Failed );
    return;
  }

  Akonadi::ItemFetchJob *fjob = dynamic_cast<Akonadi::ItemFetchJob*>( job );
  Q_ASSERT( fjob );

  mMessages =  fjob->items();
  moveMessages();
}


void MoveToTrashCommand::execute()
{
  if ( mSourceFolder.isValid() ) {
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( mSourceFolder, parent() );
    job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotFetchDone( KJob* ) ) );
  } else if ( !mMessages.isEmpty() ) {
    mSourceFolder = mMessages.first().parentCollection();
    moveMessages();
  } else {
    emitResult( OK );
  }
}

void MoveToTrashCommand::moveMessages()
{
  if ( mSourceFolder.isValid() ) {
    MoveCommand *moveCommand = new MoveCommand( findTrashFolder( mSourceFolder ), mMessages, this );
    connect( moveCommand, SIGNAL( result( Result ) ), this, SLOT( emitResult( Result ) ) );
    moveCommand->execute();
  } else
    emitResult( Failed );
}

Akonadi::Collection MoveToTrashCommand::collectionFromId(const Akonadi::Collection::Id& id) const
{
  const QModelIndex idx = Akonadi::EntityTreeModel::modelIndexForCollection(
    mModel, Akonadi::Collection(id)
  );
  return idx.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
}

Akonadi::Collection MoveToTrashCommand::trashCollectionFromResource( const Akonadi::Collection & col )
{
  //NOTE(Andras): from kmail/kmkernel.cpp
  Akonadi::Collection trashCol;
  if ( col.isValid() ) {
    if ( col.resource().contains( IMAP_RESOURCE_IDENTIFIER ) ) {
      OrgKdeAkonadiImapSettingsInterface *iface = Util::createImapSettingsInterface( col.resource() );
      if ( iface->isValid() ) {

        trashCol =  Akonadi::Collection( iface->trashCollection() );
        delete iface;
        return trashCol;
      }
      delete iface;
    }
  }
  return trashCol;
}

Akonadi::Collection MoveToTrashCommand::trashCollectionFolder()
{
  if ( the_trashCollectionFolder < 0 )
    the_trashCollectionFolder = Akonadi::SpecialMailCollections::self()->defaultCollection( Akonadi::SpecialMailCollections::Trash ).id();
  return collectionFromId( the_trashCollectionFolder );
}


Akonadi::Collection MoveToTrashCommand::findTrashFolder( const Akonadi::Collection& folder )
{
  Akonadi::Collection col = trashCollectionFromResource( folder );
  if ( !col.isValid() ) {
    col = trashCollectionFolder();
  }
  if ( folder != col )
    return col;
  return Akonadi::Collection();
}



#include "movetotrashcommand.moc"
