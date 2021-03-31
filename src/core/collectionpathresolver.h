/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "collection.h"
#include "job.h"

namespace Akonadi
{
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
    explicit CollectionPathResolver(const QString &path, QObject *parent = nullptr);

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
    explicit CollectionPathResolver(const QString &path, const Collection &parentCollection, QObject *parent = nullptr);

    /**
     * Creates a new collection path resolver to determine the path of
     * the given collection.
     *
     * @param collection The collection.
     * @param parent The parent object.
     */
    explicit CollectionPathResolver(const Collection &collection, QObject *parent = nullptr);

    /**
     * Destroys the collection path resolver.
     */
    ~CollectionPathResolver() override;

    /**
     * Returns the collection id. Only valid after the job succeeded.
     */
    Q_REQUIRED_RESULT Collection::Id collection() const;

    /**
     * Returns the collection path. Only valid after the job succeeded.
     */
    Q_REQUIRED_RESULT QString path() const;

    /**
     * Returns the path delimiter for collections.
     */
    Q_REQUIRED_RESULT static QString pathDelimiter();

protected:
    void doStart() override;

private:
    Q_DECLARE_PRIVATE(CollectionPathResolver)
};

}

