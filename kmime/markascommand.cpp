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

MarkAsCommand::MarkAsCommand( const Akonadi::MessageStatus& targetStatus, const Akonadi::Item::List& msgList, QObject* parent): CommandBase( parent )
{
  mMessages = msgList;
  mTargetStatus = targetStatus;
}

MarkAsCommand::MarkAsCommand(const Akonadi::MessageStatus &targetStatus, const Akonadi::Collection& sourceFolder, QObject* parent): CommandBase( parent )
{
  mSourceFolder = sourceFolder;
  mTargetStatus = targetStatus;
}

void MarkAsCommand::slotFetchDone(KJob* job)
{
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
}


void MarkAsCommand::execute()
{
  if ( mSourceFolder.isValid() ) {
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( mSourceFolder, parent() );
    job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotFetchDone( KJob* ) ) );
  } else if ( !mMessages.isEmpty() ) {
    mSourceFolder = mMessages.first().parentCollection();
    markMessages();
  } else {
    emitResult( OK );
  }
}

void MarkAsCommand::markMessages()
{
  mMarkJobCount = 0;
  //NOTE(Andras): from kmail/kmmcommands, KMSetStatusCommand
  foreach( const Akonadi::Item &it, mMessages ) {
    // HACK here we create a new item with an empty payload---
    //  just the Id, revision, and new flags, because otherwise
    //  non-symmetric assemble/parser in KMime might make the payload
    //  different than the original mail, and cause extra copies to be
    //  created on the server.
    Akonadi::Item item( it.id() );
    item.setRevision( it.revision() );
    // Set a custom flag
    Akonadi::MessageStatus itemStatus;
    itemStatus.setStatusFromFlags( it.flags() );

    itemStatus.set( mTargetStatus );
      /*if ( itemStatus != oldStatus )*/ {
      item.setFlags( itemStatus.statusFlags() );
      // Store back modified item
      Akonadi::ItemModifyJob *modifyJob = new Akonadi::ItemModifyJob( item, this );
      mMarkJobCount++;
      connect( modifyJob, SIGNAL( result( KJob* ) ), this, SLOT( slotModifyItemDone( KJob* ) ) );
    }
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
  if ( mMarkJobCount == 0 ) {
    emitResult( OK );
  }
}


#include "markascommand_p.moc"
