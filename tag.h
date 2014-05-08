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

#ifndef AKONADI_TAG_H
#define AKONADI_TAG_H

#include "akonadi_export.h"
#include <QString>

namespace Akonadi {
class Tag;
}

AKONADI_EXPORT uint qHash(const Akonadi::Tag &);

#include <akonadi/attributeentity.h>
#include <QVector>
#include <QSharedPointer>
#include <KUrl>
#include <QDebug>

namespace Akonadi {

/**
 * An Akonadi Tag.
 */
class AKONADI_EXPORT Tag : public AttributeEntity
{
public:
    typedef QList<Tag> List;
    typedef qint64 Id;

    /**
     * The PLAIN type has the following properties:
     * * gid == displayName
     * * immutable
     * * no hierarchy (no parent)
     *
     * PLAIN tags are general purpose tags that are easy to map by backends.
     */
    static const char *PLAIN;

    Tag();
    explicit Tag(Id id);
    /**
     * Creates a PLAIN tag
     */
    explicit Tag(const QString &name);

    Tag(const Tag &other);

    ~Tag();

    Tag &operator=(const Tag &);
    //Avoid slicing
    AttributeEntity &operator=(const AttributeEntity &);
    bool operator==(const Tag &) const;

    static Tag fromUrl(const KUrl &url);

    /**
     * Returns the url of the tag.
     */
    KUrl url() const;

    /**
     * Sets the unique @p identifier of the tag.
     */
    void setId(Id identifier);

    /**
     * Returns the unique identifier of the tag.
     */
    Id id() const;

    void setGid(const QByteArray &gid) const;
    QByteArray gid() const;

    void setRemoteId(const QByteArray &remoteId) const;
    QByteArray remoteId() const;

    void setType(const QByteArray &type) const;
    QByteArray type() const;

    void setName(const QString &name);
    QString name() const;

    void setParent(const Tag &parent);
    Tag parent() const;

    bool isValid() const;

    /**
     * Returns true if the tag is immutable (cannot be modified after creation).
     * Note that the immutability does not affect the attributes.
     */
    bool isImmutable() const;

private:
    class Private;
    QSharedPointer<Private> d;
};

}

AKONADI_EXPORT QDebug &operator<<(QDebug &debug, const Akonadi::Tag &tag);

Q_DECLARE_METATYPE(Akonadi::Tag)
Q_DECLARE_METATYPE(Akonadi::Tag::List)
Q_DECLARE_METATYPE(QSet<Akonadi::Tag>)

#endif
