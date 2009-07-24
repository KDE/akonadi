/******************************************************************************
 *
 *  File : preprocessorbase.cpp
 *  Creation date : Sun 19 Jul 2009 22:39:13
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

#include "preprocessorbase.h"

#include "agentbase_p.h"

#include "preprocessoradaptor.h"

#include <QDebug>

namespace Akonadi
{


class PreprocessorBasePrivate : public AgentBasePrivate
{
public:
  bool mInDelayedProcessing;
  qlonglong mDelayedProcessingItemId;

public:
  PreprocessorBasePrivate( PreprocessorBase *parent )
    : AgentBasePrivate( parent ),
      mInDelayedProcessing( false ),
      mDelayedProcessingItemId( 0 )
  {
  }

  Q_DECLARE_PUBLIC( PreprocessorBase )

  void delayedInit()
  {
    if ( !QDBusConnection::sessionBus().registerService( QLatin1String( "org.freedesktop.Akonadi.Preprocessor." ) + mId ) )
      kFatal() << "Unable to register service at D-Bus: " << QDBusConnection::sessionBus().lastError().message();
    AgentBasePrivate::delayedInit();
  }

}; // class PreprocessorBasePrivate


PreprocessorBase::PreprocessorBase( const QString &id )
  : AgentBase( new PreprocessorBasePrivate( this ), id )
{
  new PreprocessorAdaptor( this );
}

PreprocessorBase::~PreprocessorBase()
{

}

void PreprocessorBase::processingTerminated( ProcessingResult result )
{
  Q_D( PreprocessorBase );

  Q_ASSERT_X( result != ProcessingDelayed, "PreprocessorBase::processingTerminated", "You should never pass ProcessingDelayed to this function" );
  Q_ASSERT_X( d->mInDelayedProcessing, "PreprocessorBase::processingTerminated", "processingTerminated() called while not in delayed processing mode" );

  d->mInDelayedProcessing = false;
  emit itemProcessed( d->mDelayedProcessingItemId );
}

void PreprocessorBase::beginProcessItem( qlonglong id )
{
  Q_D( PreprocessorBase );

  qDebug() << "PreprocessorBase: about to process item " << id;

  switch( processItem( Item( id ) ) )
  {
    case ProcessingFailed:
    case ProcessingRefused:
    case ProcessingCompleted:
      qDebug() << "PreprocessorBase: item processed, emitting signal (" << id << ")";

      emit itemProcessed( id );

      qDebug() << "PreprocessorBase: item processed, signal emitted (" << id << ")";
    break;
    case ProcessingDelayed:
      qDebug() << "PreprocessorBase: item processing delayed (" << id << ")";

      d->mInDelayedProcessing = true;
      d->mDelayedProcessingItemId = id;
    break;
  }
}

} // namespace Akonadi
