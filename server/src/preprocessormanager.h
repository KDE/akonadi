/******************************************************************************
 *
 *  File : preprocessormanager.h
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

#ifndef _PREPROCESSORMANAGER_H_
#define _PREPROCESSORMANAGER_H_

#include <QObject>
#include <QList>

#include "preprocessorinstance.h"


namespace Akonadi
{

class PimItem;

/**
 * \class PreprocessorManager
 * \brief The manager for preprocessor agents
 *
 * This class takes care of synchronizing the preprocessor agents.
 *
 */
class PreprocessorManager : public QObject
{
  friend class PreprocessorInstance;

  Q_OBJECT
  Q_CLASSINFO( "D-Bus Interface", "org.freedesktop.Akonadi.PreprocessorManager" )

protected:

  /**
   * Creates an instance of PreprocessorManager
   */
  PreprocessorManager();

  /**
   * Destroys the instance of PreprocessorManager
   * and frees all the relevant resources
   */
  ~PreprocessorManager();

protected:

  /**
   * The one and only instance pointer for the class PreprocessorManager
   */
  static PreprocessorManager * mSelf;

  /**
   * The preprocessor chain.
   * The pointers inside the list are owned.
   *
   * In all the algorithms we assume that this list is actually very short
   * (say 3-4 elements) and reverse lookup (pointer->index) is really fast.
   */
  QList< PreprocessorInstance * > mPreprocessorChain;

public:

  /**
   * Returns the one and only instance pointer for the class PreprocessorManager
   * The returned pointer is valid only after a succesfull call to init()
   *
   * \sa init()
   * \sa done()
   */
  static PreprocessorManager * instance()
  {
    return mSelf;
  }

  /**
   * Initializes this class singleton by creating its one and only instance.
   * The instance is later available via the static instance() method.
   * You must call done() when you've finished using this class services.
   * Returns true upon succesfull initialisation and false when the initialization fails.
   *
   * \sa done()
   */
  static bool init();

  /**
   * Deinitializes this class singleton (if it was initialized at all)
   *
   * \sa init()
   */
  static void done();

  /**
   * Trigger the preprocessor chain for the specified Item.
   * The item has been just added to the "limbo" collection
   * (or it has been retrieved from it after a server restart)
   * and we need to pass it to each preprocessor.
   */
  void beginHandleItem( const PimItem &item );

  /**
   * This is called via D-Bus from AgentManager to register a preprocessor instance.
   */
  void registerInstance( const QString &id );

  /**
   * This is called via D-Bus from AgentManager to unregister a preprocessor instance.
   */
  void unregisterInstance( const QString &id );

  /**
   * Finds the preprocessor instance by its identifier.
   */
  PreprocessorInstance * findInstance( const QString &id );

protected:

  /**
   * This is called by PreprocessorInstance to signal that a certain preprocessor has finished
   * handling an item.
   */
  void preProcessorFinishedHandlingItem( PreprocessorInstance * preProcessor, const PimItem &item );

  /**
   * This is called internally to terminate the pre-processing
   * chain for the specified Item. All the preprocessors have
   * been triggered for it.
   */
  void endHandleItem( const PimItem &item );

}; // class PreprocessorManager

} // namespace Akonadi

#endif //!_PREPROCESSORMANAGER_H_
