/******************************************************************************
 *
 *  SPDX-FileCopyrightText: 2009 Szymon Stefanek <s.stefanek at gmail dot com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 *
 *****************************************************************************/

#pragma once

#include "agentbase.h"
#include "akonadiagentbase_export.h"
// AkonadiCore
#include <akonadi/collection.h>
#include <akonadi/item.h>

namespace Akonadi
{
class ItemFetchScope;

class PreprocessorBasePrivate;

/**
 * @short The base class for all Akonadi preprocessor agents.
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
class AKONADIAGENTBASE_EXPORT PreprocessorBase : public AgentBase
{
    Q_OBJECT

public:
    /**
     * Describes the possible return values of the processItem() method.
     */
    enum ProcessingResult {
        /**
         * Processing completed successfully for this item.
         * The Akonadi server will push in a new item when it's available.
         */
        ProcessingCompleted,

        /**
         * Processing was delayed to a later stage.
         * This must be returned when implementing asynchronous preprocessing.
         *
         * If this value is returned, finishProcessing() has to be called
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
     * This method must be implemented by every preprocessor subclass.
     * @param item the item to process
     * It must realize the preprocessing of the given @p item.
     *
     * The Akonadi server will push in for preprocessing any newly created item:
     * it's your responsibility to decide if you want to process the item or not.
     *
     * The method should return ProcessingCompleted on success, ProcessingDelayed
     * if processing is implemented asynchronously and
     * ProcessingRefused or ProcessingFailed if the processing
     * didn't complete.
     *
     * If your operation is asynchronous then you should also
     * connect to the abortRequested() signal and handle it
     * appropriately (as the server MAY abort your async job
     * if it decides that it's taking too long).
     */
    virtual ProcessingResult processItem(const Item &item) = 0;

    /**
     * This method must be called if processing is implemented asynchronously.
     * @param result the processing result
     * You should call it when you have completed the processing
     * or if an abortRequest() signal arrives (and in this case you
     * will probably use ProcessingFailed as result).
     *
     * Valid values for @p result are ProcessingCompleted,
     * PocessingRefused and ProcessingFailed. Passing any
     * other value will lead to a runtime assertion.
     */
    void finishProcessing(ProcessingResult result);

    /**
     * Sets the item fetch scope.
     *
     * The ItemFetchScope controls how much of an item's data is fetched
     * from the server, e.g. whether to fetch the full item payload or
     * only meta data.
     *
     * @param fetchScope The new scope for item fetch operations.
     *
     * @see fetchScope()
     */
    void setFetchScope(const ItemFetchScope &fetchScope);

    /**
     * Returns the item fetch scope.
     *
     * Since this returns a reference it can be used to conveniently modify the
     * current scope in-place, i.e. by calling a method on the returned reference
     * without storing it in a local variable. See the ItemFetchScope documentation
     * for an example.
     *
     * @return a reference to the current item fetch scope
     *
     * @see setFetchScope() for replacing the current item fetch scope
     */
    ItemFetchScope &fetchScope();

protected:
    /**
     * Creates a new preprocessor base agent.
     *
     * @param id The instance id of the preprocessor base agent.
     */
    PreprocessorBase(const QString &id);

    /**
     * Destroys the preprocessor base agent.
     */
    ~PreprocessorBase() override;

private:
    /// @cond PRIVATE
    Q_DECLARE_PRIVATE(PreprocessorBase)
    /// @endcond

}; // class PreprocessorBase

} // namespace Akonadi

#ifndef AKONADI_PREPROCESSOR_MAIN
/**
 * Convenience Macro for the most common main() function for Akonadi preprocessors.
 */
#define AKONADI_PREPROCESSOR_MAIN(preProcessorClass)                                                                                                           \
    int main(int argc, char **argv)                                                                                                                            \
    {                                                                                                                                                          \
        return Akonadi::PreprocessorBase::init<preProcessorClass>(argc, argv);                                                                                 \
    }
#endif //! AKONADI_RESOURCE_MAIN

