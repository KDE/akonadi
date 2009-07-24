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
 * @short The base class for all Akonadi preprocessors agents.
 *
 * This class should be used as a base class by all preprocessor agents
 * since it encapsulates large parts of the protocol between
 * preprocessor agent, agent manager and the Akonadi storage.
 *
 * Preprocessor agents are special agents that are informed about newly
 * added items before any other agents. This allows them to do filtering
 * on the items or any other task that shall be done before the new item
 * is visible in the Akonadi storage system.
 *
 * The method all the preprocessors must implement is processItem().
 *
 * @author Szymon Stefanek <s.stefanek@gmail.com>
 * @since 4.4
 */
class AKONADI_EXPORT PreprocessorBase : public AgentBase
{
  friend class PreprocessorAdaptor;

  Q_OBJECT

  public:
    /**
     * Describes the possible return values of the processItem() method.
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
       * This must be returned when implementing asynchronous preprocessing.
       *
       * If this value is returned, processingTerminated() has to be called
       * when processing is done.
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
     * This method has to be implement by every preprocessor subclass.
     *
     * Returns ProcessingCompleted on success, ProcessingDelayed
     * if processing is implemented asynchronously and
     * ProcessingRefused or ProcessingFailed if the processing
     * didn't complete.
     */
    virtual ProcessingResult processItem( const Item &item ) = 0;

    /**
     * This method must be called if processing is implemented asynchronously.
     *
     * Valid values for @p result are ProcessingCompleted,
     * PocessingRefused and ProcessingFailed. Passing any
     * other value will lead to a runtime assertion.
     */
    void processingTerminated( ProcessingResult result );

  Q_SIGNALS:
    /**
     * This signal is emitted to report item processing termination
     * to the Akonadi server.
     *
     * @note This signal is only for internal use.
     */
    void itemProcessed( qlonglong id );

  protected:
    /**
     * Creates a new preprocessor base agent.
     *
     * @param id The instance id of the preprocessor base agent.
     */
    PreprocessorBase( const QString &id );

    /**
     * Destroys the preprocessor base agent.
     */
    virtual ~PreprocessorBase();

    /**
     * This dbus method is called by the Akonadi server
     * in order to trigger the processing of an item.
     *
     * @note Do not call it manually!
     */
    void beginProcessItem( qlonglong itemId );

  private:
    // dbus Preprocessor interface
    friend class ::PreprocessorAdaptor;

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
