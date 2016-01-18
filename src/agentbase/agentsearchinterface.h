/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef AKONADI_AGENTSEARCHINTERFACE_H
#define AKONADI_AGENTSEARCHINTERFACE_H

#include "akonadiagentbase_export.h"
#include <QString>

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
        Rid
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

    void searchFinished(const QVector<qint64> &result, ResultScope scope);
    void searchFinished(const ImapSet &result, ResultScope scope);
    void searchFinished(const QVector<QByteArray> &result);
private:
    //@cond PRIVATE
    AgentSearchInterfacePrivate *const d;
    //@endcond
};

}

#endif
