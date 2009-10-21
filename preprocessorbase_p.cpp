/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "preprocessorbase_p.h"

#include "preprocessorbase.h"

#include "compoundfetchjob_p.h"
#include "preprocessoradaptor.h"

using namespace Akonadi;

PreprocessorBasePrivate::PreprocessorBasePrivate( PreprocessorBase *parent )
  : AgentBasePrivate( parent ),
    mInDelayedProcessing( false ),
    mDelayedProcessingItemId( 0 )
{
  new PreprocessorAdaptor( this );
}

void PreprocessorBasePrivate::delayedInit()
{
  if ( !QDBusConnection::sessionBus().registerService( QLatin1String( "org.freedesktop.Akonadi.Preprocessor." ) + mId ) )
    kFatal() << "Unable to register service at D-Bus: " << QDBusConnection::sessionBus().lastError().message();
  AgentBasePrivate::delayedInit();
}

void PreprocessorBasePrivate::beginProcessItem( qlonglong itemId, qlonglong collectionId, const QString &mimeType )
{
  kDebug() << "PreprocessorBase: about to process item " << itemId << " in collection " << collectionId << " with mimeType " << mimeType;

  CompoundFetchJob *fetchJob = new CompoundFetchJob( Item( static_cast<Item::Id>( itemId ) ), Collection( static_cast<Collection::Id>( collectionId ) ), this );
  connect( fetchJob, SIGNAL( result( KJob* ) ), SLOT( compoundFetched( KJob* ) ) );
  fetchJob->start();
}

void PreprocessorBasePrivate::compoundFetched( KJob *job )
{
  Q_Q( PreprocessorBase );

  if ( job->error() ) {
    emit itemProcessed( PreprocessorBase::ProcessingFailed );
    return;
  }

  CompoundFetchJob *fetchJob = qobject_cast<CompoundFetchJob*>( job );

  const Item item = fetchJob->item();
  const Collection collection = fetchJob->collection();

  switch ( q->processItem( item, collection ) ) {
    case PreprocessorBase::ProcessingFailed:
    case PreprocessorBase::ProcessingRefused:
    case PreprocessorBase::ProcessingCompleted:
      kDebug() << "PreprocessorBase: item processed, emitting signal (" << item.id() << ")";

      // TODO: Handle the different status codes appropriately

      emit itemProcessed( item.id() );

      kDebug() << "PreprocessorBase: item processed, signal emitted (" << item.id() << ")";
    break;
    case PreprocessorBase::ProcessingDelayed:
      kDebug() << "PreprocessorBase: item processing delayed (" << item.id() << ")";

      mInDelayedProcessing = true;
      mDelayedProcessingItemId = item.id();
    break;
  }
}

#include "preprocessorbase_p.moc"
#include "compoundfetchjob_p.moc"
