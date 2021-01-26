/*
    SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_ABSTRACTSEARCHPLUGIN
#define AKONADI_ABSTRACTSEARCHPLUGIN

#include <QObject>
#include <QSet>
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
