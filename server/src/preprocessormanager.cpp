/******************************************************************************
 *
 *  File : preprocessormanager.cpp
 *  Creation date : Sat 18 Jul 2009 01:58:50
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

#include "preprocessormanager.h"

#include "entities.h" // Akonadi::PimItem
#include "tracer.h"

#include "preprocessormanageradaptor.h"

#include <QtCore/QDebug>

namespace Akonadi
{

// The one and only PreprocessorManagerobject
PreprocessorManager * PreprocessorManager::mSelf = NULL;

PreprocessorManager::PreprocessorManager()
  : QObject()
{
  mSelf = this; // just to have it set early

  // Hook in our D-Bus interface "shell".
  new PreprocessorManagerAdaptor( this );

  QDBusConnection::sessionBus().registerObject(
      QLatin1String("/PreprocessorManager"),
      this,
      QDBusConnection::ExportAdaptors
    );
}

PreprocessorManager::~PreprocessorManager()
{
  // FIXME: Explicitly interrupt pre-processing here ?
  //        Pre-Processors should auto-protect themselves from re-processing an item:
  //        they are "closer to the DB" from this point of view.
  qDeleteAll( mPreprocessorChain );
}

bool PreprocessorManager::init()
{
  if( mSelf )
    return false;
  mSelf = new PreprocessorManager();
  return true;
}

void PreprocessorManager::done()
{
  if( !mSelf )
    return;
  delete mSelf;
  mSelf = NULL;
}

PreprocessorInstance * PreprocessorManager::findInstance( const QString &id )
{
  foreach( PreprocessorInstance * instance, mPreprocessorChain )
  {
    if( instance->id() == id )
      return instance;
  }

  return NULL;
}

void PreprocessorManager::registerInstance( const QString &id )
{
  qDebug() << "PreprocessorManager::registerInstance(" << id << ")";

  PreprocessorInstance * instance = findInstance( id );
  if( instance )
    return; // already registered.

  // The PreprocessorInstance objects are actually always added at the end of the queue
  // TODO: Maybe we need some kind of ordering here ?
  //       In that case we'll need to fiddle with the items that are currently enqueued for processing...

  instance = new PreprocessorInstance( id );
  if( !instance->init() )
  {
    Tracer::self()->warning(
        QLatin1String( "PreprocessorManager" ),
        QString::fromLatin1( "Could not initialize preprocessor instance '%1'" )
          .arg( id )
      );
    delete instance;
    return;
  }

  qDebug() << "Registering preprocessor instance " << id;

  mPreprocessorChain.append( instance );

  // TEST CODE
  //beginHandleItem( PimItem( 122, 1, QLatin1String("test"), 0, 0, QDateTime::currentDateTime(), QDateTime::currentDateTime(), false, 1024 ) );
  // TEST CODE
}

void PreprocessorManager::unregisterInstance( const QString &id )
{
  qDebug() << "PreprocessorManager::unregisterInstance(" << id << ")";

  PreprocessorInstance * instance = findInstance( id );
  if( !instance )
    return; // not our instance: don't complain (as we might be called for non-preprocessor agents too)

  // FIXME: All of its items in the processing chain must be queued to the next preprocessor!

  mPreprocessorChain.removeOne( instance );
  delete instance;
}

void PreprocessorManager::beginHandleItem( const PimItem &item )
{
  // This is the entry point of the pre-processing chain.

  if( mPreprocessorChain.isEmpty() )
  {
    // No preprocessors at all: immediately end handling the item.
    endHandleItem( item );
    return;
  }

  // Activate the first preprocessor.
  PreprocessorInstance * preProcessor = mPreprocessorChain.first();
  Q_ASSERT( preProcessor );

  preProcessor->enqueueItem( item );
  // the preprocessor will call our "preProcessorFinishedHandlingItem() method"
  // when done with the item.
}

void PreprocessorManager::preProcessorFinishedHandlingItem( PreprocessorInstance * preProcessor, const PimItem &item )
{
  int idx = mPreprocessorChain.indexOf( preProcessor );
  Q_ASSERT( idx >= 0 ); // must be there!

  if( idx < ( mPreprocessorChain.count() - 1 ) )
  {
    // This wasn't the last preprocessor: trigger the next one.
    PreprocessorInstance * nextPreprocessor = mPreprocessorChain[ idx + 1 ];
    Q_ASSERT( nextPreprocessor );
    Q_ASSERT( nextPreprocessor != preProcessor );

    nextPreprocessor->enqueueItem( item );
  } else {
    // This was the last preprocessor: end handling the item.
    endHandleItem( item );
  }
}

void PreprocessorManager::endHandleItem( const PimItem &item )
{
  // The exit point of the pre-processing chain.

  // TODO: Move the item to the appropriate collection.
  Q_UNUSED( item ); // for now
}

} // namespace Akonadi
