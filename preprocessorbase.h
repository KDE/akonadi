/******************************************************************************
 *
 *  File : preprocessorbase.h
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

#ifndef _PREPROCESSORBASE_H_
#define _PREPROCESSORBASE_H_

#include "akonadi_export.h"

#include <akonadi/agentbase.h>
#include <akonadi/collection.h>
#include <akonadi/item.h>

class PreprocessorAdaptor;

namespace Akonadi
{

class PreprocessorBasePrivate;

/**
 * @short The base class for all Akonadi preprocessors.
 *
 * This class should be used as a base class by all preprocessor agents
 * since it encapsulates large parts of the protocol between
 * preprocessor agent, agent manager and the Akonadi storage.
 *
 * The method all the preprocessors must implement is processItem().
 */
class AKONADI_EXPORT PreprocessorBase : public AgentBase
{
  friend class PreprocessorAdaptor;

  Q_OBJECT

public:

  /**
   * Creates an instance of a PreprocessorBase.
   * You don't need to call this explicitly if you
   * use the AKONADI_PREPROCESSOR_MAIN macro below.
   */
  PreprocessorBase( const QString &id );

  /**
   * Destroys the PreprocessorBase instance.
   */
  virtual ~PreprocessorBase();

public:

  /**
   * The possible results of your processItem() function.
   */
  enum ProcessingResult
  {
    /**
     * Processing completed succesfully for this item.
     * The Akonadi server will push in a new item when it's available.
     */
    ProcessingCompleted,

    /**
     * Processing was delayed to a later stage.
     * This is what you want to return when implementing
     * asynchronous preprocessing.
     *
     * If you return this value then you're responsible of calling
     * processingTerminated() when you're done.
     */
    ProcessingDelayed,

    /**
     * Processing for this item failed (and the failure is unrecoverable).
     * The Akonadi server will push in a new item when it's available,
     * after possibly logging the failure.
     */
    ProcessingFailed,

    /**
     * Processing for this item was refused. This is very
     * similar to ProcessingFailed above but additionally remarks
     * that the item that the Akonadi server pushed in wasn't
     * meant for this Preprocessor.
     * The Akonadi server will push in a new item when it's available,
     * after possibly logging the failure and maybe taking some additional action.
     */
    ProcessingRefused

  };

  /**
   * This is the pure virtual you need to implement
   * in your preprocessor.
   *
   * Return ProcessingCompleted on success, ProcessingDelayed
   * if you're implementing asynchronous preprocessing and
   * ProcessingRefused or ProcessingFailed if the processing
   * didn't complete.
   *
   * If you return ProcessingDelayed then you also MUST
   * call processingTerminated() when you're asynchronous
   * elaboration completes.
   */
  virtual ProcessingResult processItem( const Item &item ) = 0;

  /**
   * You need to call this function if you are implementing an asynchronous
   * preprocessor. In processItem() you trigger your async work
   * and return ProcessingDelayed. The framework will then assume
   * that you're busy until you call this function.
   *
   * Possible values for result are ProcessingCompleted,
   * PocessingRefused and ProcessingFailed. Passing ProcessingDelayed
   * is a programming error and will be punished by a Q_ASSERT().
   */
  void processingTerminated( ProcessingResult result );

private:

  // dbus Preprocessor interface: don't look :)
  friend class ::PreprocessorAdaptor;

  /**
   * Internal D-Bus handler. Called by the Akonadi server
   * in order to trigger the processing of an item. Don't touch :)
   */
  void beginProcessItem( qlonglong id );

Q_SIGNALS:
  /**
   * Internal D-Bus signal. Used to report item processing termination
   * to the Akonadi server. Don't touch :)
   */
  void itemProcessed( qlonglong id );
  
private:

  Q_DECLARE_PRIVATE( PreprocessorBase )

}; // class PreprocessorBase

} // namespace Akonadi

#ifndef AKONADI_PREPROCESSOR_MAIN
/**
 * Convenience Macro for the most common main() function for Akonadi preprocessors.
 */
#define AKONADI_PREPROCESSOR_MAIN( preProcessorClass )                       \
  int main( int argc, char **argv )                                          \
  {                                                                          \
    return Akonadi::PreprocessorBase::init<preProcessorClass>( argc, argv ); \
  }
#endif //!AKONADI_RESOURCE_MAIN


#endif //!_PREPROCESSORBASE_H_
