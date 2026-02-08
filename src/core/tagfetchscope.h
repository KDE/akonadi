/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

#include <QSharedPointer>

namespace Akonadi
{
class TagFetchScopePrivate;

/*!
 * \class Akonadi::TagFetchScope
 * \inheaderfile Akonadi/TagFetchScope
 * \inmodule AkonadiCore
 *
 * \brief Specifies which parts of a tag should be fetched from the Akonadi storage.
 *
 * \since 4.13
 */
class AKONADICORE_EXPORT TagFetchScope
{
public:
    /*!
     * Creates an empty tag fetch scope.
     *
     * Using an empty scope will only fetch the very basic meta data of tags,
     * e.g. local id, remote id and mime type
     */
    TagFetchScope();

    /*!
     * Creates a new tag fetch scope from an \a other.
     */
    TagFetchScope(const TagFetchScope &other);

    /*!
     * Destroys the tag fetch scope.
     */
    ~TagFetchScope();

    /*!
     * Assigns the \a other to this scope and returns a reference to this scope.
     */
    TagFetchScope &operator=(const TagFetchScope &other);

    /*!
     * Returns all explicitly fetched attributes.
     *
     * Undefined if fetchAllAttributes() returns true.
     *
     * \sa fetchAttribute()
     */
    [[nodiscard]] QSet<QByteArray> attributes() const;

    /*!
     * Sets whether to fetch tag remote ID.
     *
     * This option only has effect for Resources.
     */
    void setFetchRemoteId(bool fetchRemoteId);

    /*!
     * Returns whether tag remote ID should be fetched.
     */
    [[nodiscard]] bool fetchRemoteId() const;

    /*!
     * Sets whether to fetch all attributes.
     */
    void setFetchAllAttributes(bool fetchAllAttributes);

    /*!
     * Returns whether to fetch all attributes
     */
    [[nodiscard]] bool fetchAllAttributes() const;

    /*!
     * Sets whether the attribute of the given \a type should be fetched.
     *
     * \a type The attribute type to fetch.
     * \a fetch \\ true if the attribute should be fetched, \\ false otherwise.
     */
    void fetchAttribute(const QByteArray &type, bool fetch = true);

    /*!
     * Sets whether the attribute of the requested type should be fetched.
     *
     * \a fetch \\ true if the attribute should be fetched, \\ false otherwise.
     */
    template<typename T>
    inline void fetchAttribute(bool fetch = true)
    {
        T dummy;
        fetchAttribute(dummy.type(), fetch);
    }

    /*!
     * Sets whether only the id or the complete tag should be fetched.
     *
     * The default is false.
     *
     * \since 4.15
     */
    void setFetchIdOnly(bool fetchIdOnly);

    /*!
     * Sets whether only the id of the tags should be retrieved or the complete tag.
     *
     * \sa tagFetchScope()
     * \since 4.15
     */
    [[nodiscard]] bool fetchIdOnly() const;

private:
    QSharedPointer<TagFetchScopePrivate> d;
};

}

// Q_DECLARE_METATYPE(Akonadi::TagFetchScope)
