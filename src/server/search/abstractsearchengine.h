/*
    SPDX-FileCopyrightText: 2008 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_ABSTRACTSEARCHENGINE_H
#define AKONADI_ABSTRACTSEARCHENGINE_H

#include <QtGlobal>

namespace Akonadi
{
namespace Server
{
class Collection;

/**
 * Abstract interface for search engines.
 * Executed in the main thread. Must not block.
 */
class AbstractSearchEngine
{
public:
    virtual ~AbstractSearchEngine() = default;

    /**
     * Adds the given @p collection to the search.
     */
    virtual void addSearch(const Collection &collection) = 0;

    /**
     * Removes the collection with the given @p id from the search.
     */
    virtual void removeSearch(qint64 id) = 0;

protected:
    explicit AbstractSearchEngine() = default;

private:
    Q_DISABLE_COPY_MOVE(AbstractSearchEngine)
};

} // namespace Server
} // namespace Akonadi

#endif
