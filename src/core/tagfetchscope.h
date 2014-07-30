/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#ifndef TAGFETCHSCOPE_H
#define TAGFETCHSCOPE_H

#include "akonadicore_export.h"

#include <QSharedPointer>

namespace Akonadi {

/**
 * @short Specifies which parts of a tag should be fetched from the Akonadi storage.
 *
 * @since 4.13
 */
class AKONADICORE_EXPORT TagFetchScope
{
public:

    /**
     * Creates an empty tag fetch scope.
     *
     * Using an empty scope will only fetch the very basic meta data of tags,
     * e.g. local id, remote id and mime type
     */
    TagFetchScope();

    /**
     * Creates a new tag fetch scope from an @p other.
     */
    TagFetchScope(const TagFetchScope &other);

    /**
     * Destroys the tag fetch scope.
     */
    ~TagFetchScope();

    /**
     * Assigns the @p other to this scope and returns a reference to this scope.
     */
    TagFetchScope &operator=(const TagFetchScope &other);

    /**
     * Returns all explicitly fetched attributes.
     *
     * Undefined if fetchAllAttributes() returns true.
     *
     * @see fetchAttribute()
     */
    QSet<QByteArray> attributes() const;

    /**
     * Sets whether the attribute of the given @p type should be fetched.
     *
     * @param type The attribute type to fetch.
     * @param fetch @c true if the attribute should be fetched, @c false otherwise.
     */
    void fetchAttribute(const QByteArray &type, bool fetch = true);

    /**
     * Sets whether the attribute of the requested type should be fetched.
     *
     * @param fetch @c true if the attribute should be fetched, @c false otherwise.
     */
    template <typename T> inline void fetchAttribute(bool fetch = true)
    {
        T dummy;
        fetchAttribute(dummy.type(), fetch);
    }

    /**
     * Sets wether only the id or the complete tag should be fetched.
     *
     * The default is @c false.
     *
     * @since 4.15
     */
    void setFetchIdOnly(bool fetchIdOnly);

    /**
     * Sets wether only the id of the tags should be retieved or the complete tag.
     *
     * @see tagFetchScope()
     * @since 4.15
     */
    bool fetchIdOnly() const;

private:
    class Private;
    //@cond PRIVATE
    QSharedPointer<Private> d;
    //@endcond
};

}

// Q_DECLARE_METATYPE(Akonadi::TagFetchScope)

#endif
