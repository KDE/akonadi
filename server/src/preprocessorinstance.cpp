/******************************************************************************
 *
 *  File : preprocessorinstance.cpp
 *  Creation date : Sat 18 Jul 2009 02:50:39
 *
 *  Copyright (c) 2009 Szymon Stefanek <s.stefanek at gmail dot com>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA, 02110-1301, USA.
 *
 *****************************************************************************/

#include "preprocessorinstance.h"
#include "preprocessorinterface.h"
#include "preprocessormanager.h"

#include "tracer.h"

#include <QtCore/QTimer>

const int gProcessingTimeoutInSecs = 300; // 5 minutes

namespace Akonadi
{

PreprocessorInstance::PreprocessorInstance( const QString &id )
  : QObject(), mId( id ), mInterface( 0 )
{
  Q_ASSERT( !id.isEmpty() );

  mProcessingDeadlineTimer = new QTimer( this );
  mProcessingDeadlineTimer->setSingleShot( true );
  QObject::connect(
      mProcessingDeadlineTimer,
      SIGNAL( timeout() ),
      this,
      SLOT( processingTimedOut() )
    );
  mBusy = false;
}

PreprocessorInstance::~PreprocessorInstance()
{
  if( mProcessingDeadlineTimer->isActive() )
    mProcessingDeadlineTimer->stop();
}

bool PreprocessorInstance::init()
{
  Q_ASSERT( !mBusy ); // must be called very early
  Q_ASSERT( !mInterface );

  mInterface = new OrgFreedesktopAkonadiPreprocessorInterface(
      QLatin1String( "org.freedesktop.Akonadi.Preprocessor." ) + mId,
      QLatin1String( "/" ),
      QDBusConnection::sessionBus(),
      this
    );

  if( !mInterface || !mInterface->isValid() )
  {
    Tracer::self()->warning(
        QLatin1String( "PreprocessorInstance" ),
        QString::fromLatin1( "Could not connect to pre-processor instance '%1': %2" )
          .arg( mId )
          .arg( mInterface ? mInterface->lastError().message() : QString() )
      );
    delete mInterface;
    mInterface = 0;
    return false;
  }

  QObject::connect(
      mInterface,
      SIGNAL( itemProcessed( qlonglong ) ),
      this,
      SLOT( itemProcessed( qlonglong ) )
    );

  return true;
}

void PreprocessorInstance::enqueueItem( const PimItem &item )
{
  qDebug() << "PreprocessorInstance::enqueueItem(" << item.id() << ")";

  mItemQueue.append( item );

  // If the preprocessor is already busy processing another item then do nothing.
  if ( mBusy )
  {
    // The "head" item is the one being processed and we have just added another one.
    Q_ASSERT( mItemQueue.count() > 1 );
    return;
  }

  // should have no deadline now
  Q_ASSERT( !mProcessingDeadlineTimer->isActive() );

  // Not busy: handle the item.
  processHeadItem();
}

void PreprocessorInstance::processHeadItem()
{
  // We shouldn't be called if there are no items in the queue
  Q_ASSERT( mItemQueue.count() > 0 );
  // We shouldn't be here with no interface
  Q_ASSERT( mInterface );

  mBusy = true;

  PimItem item = mItemQueue.first();

  qDebug() << "PreprocessorInstance::processHeadItem(): about to begin processing item " << item.id();

  mProcessingDeadlineTimer->start( gProcessingTimeoutInSecs * 1000 );

  // The beginProcessItem() D-Bus call is asynchronous (marked with NoReply attribute)
  mInterface->beginProcessItem( item.id() );

  qDebug() << "PreprocessorInstance::processHeadItem(): processing started for item " << item.id();

}

void PreprocessorInstance::processingTimedOut()
{
  qDebug() << "PreprocessorInstance::processingTimedOut!";

  Q_ASSERT( mProcessingDeadlineTimer->isActive() ); // we should be called from the QTimer here

  mProcessingDeadlineTimer->stop();

  // FIXME: And what now ? Try a ping() on the interface and if it fails then kill the preprocessor ?  
}

void PreprocessorInstance::itemProcessed( qlonglong id )
{
  qDebug() << "PreprocessorInstance::itemProcessed(" << id << ")";

  // We shouldn't be called if there are no items in the queue
  if( mItemQueue.count() < 1 )
  {
    Tracer::self()->warning(
        QLatin1String( "PreprocessorInstance" ),
        QString::fromLatin1( "Pre-processor instance '%1' emitted itemProcessed(%2) but we actually have no item in the queue" )
          .arg( mId )
          .arg( id )
      );
    return; // preprocessor is buggy (FIXME: What now ?)
  }

  // We should be busy now: this is more likely our fault, not the preprocessor's one.
  Q_ASSERT( mBusy );

  PimItem item = mItemQueue.first();

  if( item.id() != id )
  {
    Tracer::self()->warning(
        QLatin1String( "PreprocessorInstance" ),
        QString::fromLatin1( "Pre-processor instance '%1' emitted itemProcessed(%2) but the head item in the queue has id %3" )
          .arg( mId )
          .arg( id )
          .arg( item.id() )
      );

    // FIXME: And what now ?
  }

  mProcessingDeadlineTimer->stop();

  mItemQueue.removeFirst();

  PreprocessorManager::instance()->preProcessorFinishedHandlingItem( this, item );

  if( mItemQueue.count() < 1 )
  {
    // Nothing more to do
    mBusy = false;
    return;
  }

  // Stay busy and process next item in the queue
  processHeadItem();
}


} // namespace Akonadi

