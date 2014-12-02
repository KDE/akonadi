/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTIONPATHRESOLVER_P_H
#define AKONADI_COLLECTIONPATHRESOLVER_P_H

#include "akonadicore_export.h"
#include "collection.h"
#include "job.h"

namespace Akonadi {

class CollectionPathResolverPrivate;

/**
 * @internal
 *
 * Converts between collection id and collection path.
 *
 * While it is generally recommended to use collection ids, it can
 * be necessary in some cases (eg. a command line client) to use the
 * collection path instead. Use this class to get a collection id
 * from a collection path.
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT CollectionPathResolver : public Job
{
    Q_OBJECT

public:
    /**
     * Creates a new collection path resolver to convert a path into a id.
     *
     * Equivalent to calling CollectionPathResolver(path, Collection:root(), parent)
     *
     * @param path The collection path.
     * @param parent The parent object.
     */
    explicit CollectionPathResolver(const QString &path, QObject *parent = Q_NULLPTR);

    /**
     * Create a new collection path resolver to convert a path into an id.
     *
     * The @p path is resolved relatively to @p parentCollection. This can be
     * useful for resource, which now the root collection.
     *
     * @param path The collection path.
     * @param parentCollection Collection relatively to which the path will be resolved.
     * @param parent The parent object.
     *
     * @since 4.14
     */
    explicit CollectionPathResolver(const QString &path, const Collection &parentCollection, QObject *parent = Q_NULLPTR);

    /**
     * Creates a new collection path resolver to determine the path of
     * the given collection.
     *
     * @param collection The collection.
     * @param parent The parent object.
     */
    explicit CollectionPathResolver(const Collection &collection, QObject *parent = Q_NULLPTR);

    /**
     * Destroys the collection path resolver.
     */
    ~CollectionPathResolver();

    /**
     * Returns the collection id. Only valid after the job succeeded.
     */
    Collection::Id collection() const;

    /**
     * Returns the collection path. Only valid after the job succeeded.
     */
    QString path() const;

    /**
     * Returns the path delimiter for collections.
     */
    static QString pathDelimiter();

protected:
    void doStart();

private:
    Q_DECLARE_PRIVATE(CollectionPathResolver)

    //@cond PRIVATE
    Q_PRIVATE_SLOT(d_func(), void jobResult(KJob *))
    //@endcond
};

}

#endif
