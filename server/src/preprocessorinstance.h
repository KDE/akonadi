/******************************************************************************
 *
 *  File : preprocessorinstance.h
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

#ifndef _PREPROCESSORINSTANCE_H_
#define _PREPROCESSORINSTANCE_H_

#include "entities.h"

#include <QObject>

class OrgFreedesktopAkonadiPreprocessorInterface;

class QTimer;

namespace Akonadi
{

class AgentInstance;

/**
 * A single preprocessor (agent) instance.
 *
 * Most of the interface of this class is protected and is exposed only
 * to PreprocessorManager (singleton).
 */
class PreprocessorInstance : public QObject
{
  friend class PreprocessorManager;

  Q_OBJECT
public:

  /**
   * Create an instance of a PreprocessorInstance descriptor.
   */
  PreprocessorInstance( const QString &id );

  /**
   * Destroy this instance of the PreprocessorInstance descriptor.
   */
  ~PreprocessorInstance();

protected:

  /**
   * The internal item queue.
   * The head item in the queue is the one currently being processed.
   * The other ones are waiting.
   */
  PimItem::List mItemQueue;

  /**
   * Is this processor busy ?
   * This, in fact, *should* be equivalent to "mItemQueue.count() > 0"
   * as the head item in the queue is the one being processed now.
   */
  bool mBusy;

  /**
   * The id of this preprocessor instance. This is actually
   * the AgentInstance identifier.
   */
  QString mId;

  /**
   * The preprocessor D-Bus interface. Owned.
   */
  OrgFreedesktopAkonadiPreprocessorInterface * mInterface;

  /**
   * This is used to timeout a processing on the agent side
   * which takes *really* too long.
   */
  QTimer * mProcessingDeadlineTimer;

public:

  /**
   * This is called by PreprocessorManager just after the construction
   * in order to connect to the preprocessor instance via D-Bus.
   * In case of failure this object should be destroyed as it can't
   * operate properly. The error message is printed via Tracer.
   */
  bool init();

  /**
   * Returns true if this preprocessor instance is currently processing an item.
   * That is: if we have called "processItem()" on it and it hasn't emitted
   * itemProcessed() yet.
   */
  bool isBusy() const
  {
    return mBusy;
  }

  /**
   * Returns the id of this preprocessor. This is actually
   * the AgentInstance identifier but it's not a requirement.
   */
  const QString & id() const
  {
    return mId;
  }

protected:

  /**
   * This is called by PreprocessorManager to enqueue a PimItem
   * for processing by this preprocessor instance.
   */
  void enqueueItem( const PimItem &item );

  /**
   * This function starts processing of the first item in mItemQueue.
   */
  void processHeadItem();

private Q_SLOTS:
  /**
   * This is invoked to signal that the processing of the current (head)
   * item has terminated and the next item should be processed.
   */
  void itemProcessed( qlonglong id );

  /**
   * Triggered when the processing operation took *really* too much time.
   */
  void processingTimedOut();

}; // class PreprocessorInstance

} // namespace Akonadi

#endif //!_PREPROCESSORINSTANCE_H_
