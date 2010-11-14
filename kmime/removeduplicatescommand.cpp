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


#include "removeduplicatescommand_p.h"
#include "movetotrashcommand_p.h"
#include "util_p.h"

#include "akonadi/itemfetchjob.h"
#include "akonadi/itemfetchscope.h"
#include "kmime/kmime_message.h"

RemoveDuplicatesCommand::RemoveDuplicatesCommand( const QAbstractItemModel* model, const Akonadi::Collection::List& folders, QObject* parent ) :
  CommandBase( parent )
{
  mModel = model;
  mFolders = folders;
  mJobCount = mFolders.size();
}

void RemoveDuplicatesCommand::execute()
{
    if ( mJobCount <= 0 ) {
      emitResult( OK );
      return;
    }
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( mFolders[ mJobCount - 1] , parent() );
    job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
    job->fetchScope().fetchFullPayload();
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotFetchDone( KJob* ) ) );
}

void RemoveDuplicatesCommand::slotFetchDone( KJob* job )
{
  mJobCount--;
  if ( job->error() ) {
    // handle errors
    Util::showJobError(job);
    emitResult( Failed );
    return;
  } 
  Akonadi::ItemFetchJob *fjob = dynamic_cast<Akonadi::ItemFetchJob*>( job );
  Q_ASSERT( fjob );
  Akonadi::Item::List items = fjob->items();

  //find duplicate mails with the same messageid
  //if duplicates are found, check the content as well to be sure they are the same
  QMap<QByteArray, uint> messageIds;
  QMap<uint, QList<uint> > duplicates;
  QMap<uint, uint> bodyHashes;
  for ( int i = 0; i < items.size(); ++i ) {
    Akonadi::Item item = items[i];
    if ( item.hasPayload<KMime::Message::Ptr>() ) {
      KMime::Message::Ptr message = item.payload<KMime::Message::Ptr>();
      QByteArray idStr = message->messageID()->as7BitString( false );
      //TODO: Maybe do some more check in case of idStr.isEmpty()
      //like when the first message's body is different from the 2nd,
      //but the 2nd is the same as the 3rd, etc.
      //if ( !idStr.isEmpty() )
      {
        if ( messageIds.contains( idStr ) ) {
          uint mainId = messageIds.value( idStr );
          if ( !bodyHashes.contains( mainId ) )
            bodyHashes[ mainId ] = qHash( items[mainId].payload<KMime::Message::Ptr>()->encodedContent() );
          uint hash = qHash( message->encodedContent() );
          qDebug() << idStr << bodyHashes[ mainId ] << hash;
          if ( bodyHashes[ mainId ] == hash )
            duplicates[ mainId ].append( i );
        } else {
          messageIds[ idStr ] = i;
        }
      }
    }
  }

  for(  QMap<uint, QList<uint> >::iterator it = duplicates.begin(); it != duplicates.end(); ++it ) {    
    for (QList<uint>::iterator dupIt = it.value().begin(); dupIt != it.value().end(); ++dupIt ) {
      mDuplicateItems.append( items[*dupIt] );
    }
  }

  if ( mJobCount > 0 ) {
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( mFolders[ mJobCount - 1 ] , parent() );
    job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
    job->fetchScope().fetchFullPayload();
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotFetchDone( KJob* ) ) );    
  } else {
    MoveToTrashCommand *trashCmd = new MoveToTrashCommand( mModel, mDuplicateItems, parent() );
    connect( trashCmd, SIGNAL(result(Result)), this, SLOT(emitResult(Result)) );
    trashCmd->execute();
  }
}

