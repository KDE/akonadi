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


#include "markascommand_p.h"
#include "util_p.h"
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>

MarkAsCommand::MarkAsCommand( const Akonadi::MessageStatus& targetStatus, const Akonadi::Item::List& msgList, bool invert, QObject* parent): CommandBase( parent )
{
  mInvertMark = invert;
  mMessages = msgList;
  mTargetStatus = targetStatus;
  mFolderListJobCount = 0;
}

MarkAsCommand::MarkAsCommand(const Akonadi::MessageStatus &targetStatus, const Akonadi::Collection::List& folders, bool invert, QObject* parent): CommandBase( parent )
{
  mInvertMark = invert;
  mFolders = folders;
  mTargetStatus = targetStatus;
  mFolderListJobCount = mFolders.size();  
}

void MarkAsCommand::slotFetchDone(KJob* job)
{
  mFolderListJobCount--;

  if ( job->error() ) {
    // handle errors
    Util::showJobError(job);
    emitResult( Failed );
    return;
  }

  Akonadi::ItemFetchJob *fjob = dynamic_cast<Akonadi::ItemFetchJob*>( job );
  Q_ASSERT( fjob );

  foreach( const Akonadi::Item &item, fjob->items() ) {
    Akonadi::MessageStatus status;
    status.setStatusFromFlags( item.flags() );
    if ( mInvertMark ) {
      if ( status & mTargetStatus ) {
        mMessages.append( item );
      }      
    } else 
      if (! (status & mTargetStatus) )
      {
        mMessages.append( item );
      }
  }
  if ( mMessages.empty() ) {
    emitResult( OK );
    return;
  }

  markMessages();

  if ( mFolderListJobCount > 0 ) {
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( mFolders[mFolderListJobCount - 1], parent() );
    job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotFetchDone( KJob* ) ) );
  }
}


void MarkAsCommand::execute()
{
  if ( !mFolders.isEmpty() ) {
    //yes, we go backwards, shouldn't matter
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( mFolders[mFolderListJobCount - 1], parent() );
    job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotFetchDone( KJob* ) ) );
  } else if ( !mMessages.isEmpty() ) {
    mFolders << mMessages.first().parentCollection();
    markMessages();
  } else {
    emitResult( OK );
  }
}

void MarkAsCommand::markMessages()
{
  mMarkJobCount = 0;

  foreach( const Akonadi::Item &it, mMessages ) {
    Akonadi::Item item( it );

    // Set a custom flag
    Akonadi::MessageStatus itemStatus;
    itemStatus.setStatusFromFlags( it.flags() );

    if ( mInvertMark ) {
      
      if ( mTargetStatus & Akonadi::MessageStatus::statusRead() ) {
        itemStatus.setUnread();
      } else if ( mTargetStatus & Akonadi::MessageStatus::statusUnread() ) {
        itemStatus.setRead();
      } else
        itemStatus.toggle( mTargetStatus );
    } else {
      itemStatus.set( mTargetStatus );
    }

    item.setFlags( itemStatus.statusFlags() );
    // Store back modified item
    Akonadi::ItemModifyJob *modifyJob = new Akonadi::ItemModifyJob( item, this );
    modifyJob->setIgnorePayload( true );
    mMarkJobCount++;
    connect( modifyJob, SIGNAL( result( KJob* ) ), this, SLOT( slotModifyItemDone( KJob* ) ) );
  }
}

void MarkAsCommand::slotModifyItemDone( KJob * job )
{
  mMarkJobCount--;
  //NOTE(Andras): from kmail/kmmcommands, KMSetStatusCommand
  if ( job->error() ) {
    kDebug()<<" Error trying to set item status:" << job->errorText();
    emitResult( Failed );
  }
  if ( mMarkJobCount == 0 && mFolderListJobCount == 0 ) {
    emitResult( OK );
  }
}


#include "markascommand_p.moc"
