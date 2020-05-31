/*
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#ifndef AKONADI_ABSTRACTSEARCHPLUGIN
#define AKONADI_ABSTRACTSEARCHPLUGIN

#include <QSet>
#include <QObject>
#include <QStringList>

namespace Akonadi
{

/**
 * @class AbstractSearchPlugin
 *
 * 3rd party applications can install a search plugin for Akonadi server to
 * provide access to their search capability.
 *
 * When the server performs a search, it will send the query to all available
 * search plugins and merge the results.
 *
 * @since 1.12
 */
class AbstractSearchPlugin
{

public:
    /**
     * Destructor.
     */
    virtual ~AbstractSearchPlugin() = default;

    /**
     * Reimplement this method to provide the actual search capability.
     *
     * The implementation can block.
     *
     * @param query Search query to execute.
     * @return List of Akonadi Item IDs referring to items that are matching
     *         the query.
     */
    virtual QSet<qint64> search(const QString &query, const QVector<qint64> &collections, const QStringList &mimeTypes) = 0;

protected:
    explicit AbstractSearchPlugin() = default;

private:
    Q_DISABLE_COPY_MOVE(AbstractSearchPlugin)
};

}

Q_DECLARE_INTERFACE(Akonadi::AbstractSearchPlugin, "org.freedesktop.Akonadi.AbstractSearchPlugin")

#endif
