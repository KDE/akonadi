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

#ifndef AKONADI_PREPROCESSORMANAGER_H
#define AKONADI_PREPROCESSORMANAGER_H

#include <QObject>
#include <QList>
#include <QHash>

#include <deque>

class QTimer;
class QMutex;

#include "preprocessorinstance.h"

namespace Akonadi
{
namespace Server
{

class PimItem;
class DataStore;

/**
 * \class PreprocessorManager
 * \brief The manager for preprocessor agents
 *
 * This class takes care of synchronizing the preprocessor agents.
 *
 * The preprocessors see the incoming PimItem objects before the user
 * can see them (as long as the UI applications honor the hidden attribute).
 * The items are marked as hidden (by the Append and AkAppend
 * handlers) and then enqueued to the preprocessor chain via this class.
 * Once all the preprocessors have done their work the item is unhidden again.
 *
 * Preprocessing isn't designed for critical tasks. There may
 * be circumstances under that the Akonadi server fails to push an item
 * to all the preprocessors. Most notably after a server restart all
 * the items for that preprocessing was interrupted are just unhidden
 * without any attempt to resume the preprocessor jobs.
 *
 * The enqueue requests may or may not arrive from "inside" a database
 * transaction. The uncommitted transaction would "hide" the newly created items
 * from the preprocessor instances (which are separate processes).
 * This class, then, takes care of holding the newly arrived items
 * in a wait queue until their transaction is committed (or rolled back).
 */
class PreprocessorManager : public QObject
{
    friend class PreprocessorInstance;

    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Akonadi.PreprocessorManager")

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
    static PreprocessorManager *mSelf;

    /**
     * The hashtable of transaction wait queues. There is one wait
     * queue for each DataStore that is currently in a transaction.
     */
    QHash< const DataStore *, std::deque< qint64 > *> mTransactionWaitQueueHash;

    /**
     * The preprocessor chain.
     * The pointers inside the list are owned.
     *
     * In all the algorithms we assume that this list is actually very short
     * (say 3-4 elements) and reverse lookup (pointer->index) is really fast.
     */
    QList< PreprocessorInstance *> mPreprocessorChain;

    /**
     * Is preprocessing enabled at all in this Akonadi server instance?
     * This is true by default and can be set via setEnabled().
     * Mainly used to disable preprocessing via configuration file.
     */
    bool mEnabled;

    /**
     * The mutex used to protect the internals of this class  (mainly
     * the mPreprocessorChain member).
     */
    QMutex *mMutex;

    /**
     * The heartbeat timer. Used mainly to expire preprocessor jobs.
     */
    QTimer *mHeartbeatTimer;

public:

    /**
     * Returns the one and only instance pointer for the class PreprocessorManager
     * The returned pointer is valid only after a successful call to init().
     *
     * \sa init()
     * \sa done()
     */
    static PreprocessorManager *instance()
    {
        return mSelf;
    }

    /**
     * Initializes this class singleton by creating its one and only instance.
     * This is actually called in the AkonadiServer constructor.
     *
     * The instance is later available via the static instance() method.
     * You must call done() when you've finished using this class services.
     * Returns true upon successful initialisation and false when the initialization fails.
     *
     * \sa done()
     */
    static bool init();

    /**
     * Deinitializes this class singleton (if it was initialized at all).
     * This is actually called in the AkonadiServer::quit() method.
     *
     * \sa init()
     */
    static void done();

    /**
     * Returns true if preprocessing is active in this Akonadi server.
     * This means that we have at least one active preprocessor and
     * preprocessing hasn't been explicitly disabled via configuration
     * (so if isActive() returns true then also isEnabled() will return true).
     *
     * This function is thread-safe.
     */
    bool isActive();

    /**
     * Returns true if this preprocessor hasn't been explicitly disabled
     * via setEnabled( false ). This is used to disable preprocessing
     * via configuration even if we have a valid chain of preprocessors.
     *
     * Please note that this flag doesn't tell if we actually have
     * some registered preprocessors and thus we can do some meaningful job.
     * You should use isActive() for this purpose.
     */
    bool isEnabled() const
    {
        return mEnabled;
    }

    /**
     * Explicitly enables or disables the preprocessing in this Akonadi server.
     * The PreprocessorManager starts in enabled state but can be disabled
     * at a later stage: this is mainly used to disable preprocessing via
     * configuration.
     *
     * Please note that setting this to true doesn't interrupt the currently
     * running preprocessing jobs. Anything that was enqueued will be processed
     * anyway. However, in Akonadi this is only invoked very early,
     * when no preprocessors are alive yet.
     */
    void setEnabled(bool enabled)
    {
        mEnabled = enabled;
    }

    /**
     * Trigger the preprocessor chain for the specified item.
     * The item should have been added to the Akonadi database via
     * the specified DataStore object. If the DataStore is in a
     * transaction then this class will put the item in a wait
     * queue until the transaction is committed. If the transaction
     * is rolled back the whole wait queue will be discarded.
     * If the DataStore is not in a transaction then the item
     * will be pushed directly to the preprocessing chain.
     *
     * You should make sure that the preprocessor chain isActive()
     * before calling this method. The items you pass to this method,
     * also, should have the hidden attribute set.
     *
     * This function is thread-safe.
     */
    void beginHandleItem(const PimItem &item, const DataStore *dataStore);

    /**
     * This is called via D-Bus from AgentManager to register a preprocessor instance.
     *
     * This function is thread-safe.
     */
    void registerInstance(const QString &id);

    /**
     * This is called via D-Bus from AgentManager to unregister a preprocessor instance.
     *
     * This function is thread-safe.
     */
    void unregisterInstance(const QString &id);

protected:

    /**
     * This is called by PreprocessorInstance to signal that a certain preprocessor has finished
     * handling an item.
     *
     * This function is thread-safe.
     */
    void preProcessorFinishedHandlingItem(PreprocessorInstance *preProcessor, qint64 itemId);

private:

    /**
     * Finds the preprocessor instance by its identifier.
     *
     * This must be called with mMutex locked.
     */
    PreprocessorInstance *lockedFindInstance(const QString &id);

    /**
     * Pushes the specified item to the first preprocessor.
     * The caller *MUST* make sure that there is at least one preprocessor in the chain.
     */
    void lockedActivateFirstPreprocessor(qint64 itemId);

    /**
     * This is called internally to terminate the pre-processing
     * chain for the specified Item. All the preprocessors have
     * been triggered for it.
     *
     * This must be called with mMutex locked.
     */
    void lockedEndHandleItem(qint64 itemId);

    /**
     * This is the unprotected core of the unregisterInstance() function above.
     */
    void lockedUnregisterInstance(const QString &id);

    /**
     * Kill the wait queue for the specific DataStore object.
     */
    void lockedKillWaitQueue(const DataStore *dataStore, bool disconnectSlots);

private Q_SLOTS:

    /**
     * Connected to the mHeartbeatTimer. Triggered every minute or something like that :D
     * Mainly used to expire preprocessor jobs.
     */
    void heartbeat();

    /**
     * This is used to handle database transactions and wait queues.
     * The call to this slot usually comes from a queued signal/slot connection
     * (i.e. from the *Append handler thread).
     */
    void dataStoreDestroyed();

    /**
     * This is used to handle database transactions and wait queues.
     * The call to this slot usually comes from a queued signal/slot connection
     * (i.e. from the *Append handler thread).
     */
    void dataStoreTransactionCommitted();

    /**
     * This is used to handle database transactions and wait queues.
     * The call to this slot usually comes from a queued signal/slot connection
     * (i.e. from the *Append handler thread).
     */
    void dataStoreTransactionRolledBack();

}; // class PreprocessorManager

} // namespace Server
} // namespace Akonadi

#endif //!_PREPROCESSORMANAGER_H_
