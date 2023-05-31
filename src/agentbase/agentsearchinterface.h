/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiagentbase_export.h"
#include <QString>

#include <memory>

namespace Akonadi
{
class Collection;
class AgentSearchInterfacePrivate;
class ImapSet;

/**
 * @short An interface for agents (or resources) that support searching in their backend.
 *
 * Inherit from this additionally to Akonadi::AgentBase (or Akonadi::ResourceBase)
 * and implement its two pure virtual methods.
 *
 * Make sure to add the @c Search capability to the agent desktop file.
 *
 * @since 4.5
 */
class AKONADIAGENTBASE_EXPORT AgentSearchInterface
{
public:
    enum ResultScope {
        Uid,
        Rid,
    };

    /**
     * Creates a new agent search interface.
     */
    AgentSearchInterface();

    /**
     * Destroys the agent search interface.
     */
    virtual ~AgentSearchInterface();

    /**
     * Adds a new search.
     *
     * @param query The query string, using the language specified in @p queryLanguage
     * @param queryLanguage The query language used for @p query
     * @param resultCollection The destination collection for the search results. It's a virtual
     * collection, results can be added/removed using Akonadi::LinkJob and Akonadi::UnlinkJob respectively.
     */
    virtual void addSearch(const QString &query, const QString &queryLanguage, const Akonadi::Collection &resultCollection) = 0;

    /**
     * Removes a previously added search.
     * @param resultCollection The result collection given in an previous addSearch() call.
     * You do not need to take care of deleting results in there, the collection is just provided as a way to
     * identify the search.
     */
    virtual void removeSearch(const Akonadi::Collection &resultCollection) = 0;

    /**
     * Perform a search on remote storage and return results using SearchResultJob.
     *
     * @since 4.13
     */
    virtual void search(const QString &query, const Collection &collection) = 0;

    void searchFinished(const QList<qint64> &result, ResultScope scope);
    void searchFinished(const ImapSet &result, ResultScope scope);
    void searchFinished(const QList<QByteArray> &result);

private:
    /// @cond PRIVATE
    std::unique_ptr<AgentSearchInterfacePrivate> const d;
    /// @endcond
};

}
