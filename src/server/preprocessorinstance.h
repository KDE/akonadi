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

#ifndef AKONADI_PREPROCESSORINSTANCE_H
#define AKONADI_PREPROCESSORINSTANCE_H

#include <QtCore/QObject>
#include <QtCore/QDateTime>

#include <deque>

class OrgFreedesktopAkonadiPreprocessorInterface;

namespace Akonadi {
namespace Server {

class AgentInstance;

/**
 * A single preprocessor (agent) instance.
 *
 * Most of the interface of this class is protected and is exposed only
 * to PreprocessorManager (singleton).
 *
 * This class is NOT thread safe. The caller is responsible of protecting
 * agains concurrent access.
 */
class PreprocessorInstance : public QObject
{
    friend class PreprocessorManager;

    Q_OBJECT

protected:

    /**
     * Create an instance of a PreprocessorInstance descriptor.
     */
    PreprocessorInstance(const QString &id);

public: // This is public only for qDeleteAll() called from PreprocessorManager
    // ...for some reason couldn't convince gcc to have it as friend...

    /**
     * Destroy this instance of the PreprocessorInstance descriptor.
     */
    ~PreprocessorInstance();

private:

    /**
     * The internal queue if item identifiers.
     * The head item in the queue is the one currently being processed.
     * The other ones are waiting.
     */
    std::deque< qint64 > mItemQueue;

    /**
     * Is this processor busy ?
     * This, in fact, *should* be equivalent to "mItemQueue.count() > 0"
     * as the head item in the queue is the one being processed now.
     */
    bool mBusy;

    /**
     * The date-time at that we have started processing the current
     * item in the queue. This is used to compute the processing time
     * and eventually spot a "dead" preprocessor (which takes longer
     * than N minutes to process an item).
     */
    QDateTime mItemProcessingStartDateTime;

    /**
     * The id of this preprocessor instance. This is actually
     * the AgentInstance identifier.
     */
    QString mId;

    /**
     * The preprocessor D-Bus interface. Owned.
     */
    OrgFreedesktopAkonadiPreprocessorInterface *mInterface;

protected:

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
     * Returns the time in seconds elapsed since the current item was submitted
     * to the slave preprocessor instance. If no item is currently being
     * processed then this function returns -1;
     */
    int currentProcessingTime();

    /**
     * Returns the id of this preprocessor. This is actually
     * the AgentInstance identifier but it's not a requirement.
     */
    const QString &id() const
    {
        return mId;
    }

    /**
     * Returns a pointer to the internal preprocessor instance
     * item queue. Don't mess with it unless you *really* know
     * what you're doing. Use enqueueItem() to add an item
     * to the queue. This method is provided to the PreprocessorManager
     * to take over the item queue of a dying preprocessor.
     *
     * The returned pointer is granted to be non null.
     */
    std::deque< qint64 > *itemQueue()
    {
        return &mItemQueue;
    }

    /**
     * This is called by PreprocessorManager to enqueue a PimItem
     * for processing by this preprocessor instance.
     */
    void enqueueItem(qint64 itemId);

    /**
     * Attempts to abort the processing of the current item.
     * May be called only if isBusy() returns true and an assertion
     * will remind you of that.
     * Returns true if the abort request was successfully sent
     * (but not necessarily handled by the slave) and false
     * if the request couldn't be sent for some reason.
     */
    bool abortProcessing();

    /**
     * Attempts to invoke the preprocessor slave restart via
     * AgentManager. This is the "last resort" action before
     * starting to ignore the preprocessor (after it misbehaved).
     */
    bool invokeRestart();

private:

    /**
     * This function starts processing of the first item in mItemQueue.
     * It's only used internally.
     */
    void processHeadItem();

private Q_SLOTS:

    /**
     * This is invoked to signal that the processing of the current (head)
     * item has terminated and the next item should be processed.
     */
    void itemProcessed(qlonglong id);

}; // class PreprocessorInstance

} // namespace Server
} // namespace Akonadi

#endif //!_PREPROCESSORINSTANCE_H_
