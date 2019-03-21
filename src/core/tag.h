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

#include "akonadicore_export.h"
#include "attribute.h"

#include <QString>
#include <QSharedPointer>
#include <QUrl>
#include <QDebug>

namespace Akonadi
{
class TagModifyJob;
class TagPrivate;

/**
 * An Akonadi Tag.
 */
class AKONADICORE_EXPORT Tag
{
public:
    typedef QVector<Tag> List;
    typedef qint64 Id;

    /**
     * The PLAIN type has the following properties:
     * * gid == displayName
     * * immutable
     * * no hierarchy (no parent)
     *
     * PLAIN tags are general purpose tags that are easy to map by backends.
     */
    static const char PLAIN[];

    /**
     * The GENERIC type has the following properties:
     * * mutable
     * * gid is RFC 4122 compatible
     * * no hierarchy (no parent)
     *
     * GENERIC tags are general purpose tags, that are used, if you can change tag name.
     */
    static const char GENERIC[];

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
    bool operator==(const Tag &) const;
    bool operator!=(const Tag &) const;

    static Tag fromUrl(const QUrl &url);

    /**
     * Adds an attribute to the entity.
     *
     * If an attribute of the same type name already exists, it is deleted and
     * replaced with the new one.
     *
     * @param attribute The new attribute.
     *
     * @note The entity takes the ownership of the attribute.
     */
    void addAttribute(Attribute *attribute);

    /**
     * Removes and deletes the attribute of the given type @p name.
     */
    void removeAttribute(const QByteArray &name);

    /**
     * Returns @c true if the entity has an attribute of the given type @p name,
     * false otherwise.
     */
    bool hasAttribute(const QByteArray &name) const;

    /**
     * Returns a list of all attributes of the entity.
     */
    Attribute::List attributes() const;

    /**
     * Removes and deletes all attributes of the entity.
     */
    void clearAttributes();

    /**
     * Returns the attribute of the given type @p name if available, 0 otherwise.
     */
    const Attribute *attribute(const QByteArray &name) const;
    Attribute *attribute(const QByteArray &name);

    /**
     * Describes the options that can be passed to access attributes.
     */
    enum CreateOption {
        AddIfMissing,    ///< Creates the attribute if it is missing
        DontCreate       ///< Does not create an attribute if it is missing (default)
    };

    /**
     * Returns the attribute of the requested type.
     * If the entity has no attribute of that type yet, a new one
     * is created and added to the entity.
     *
     * @param option The create options.
     */
    template <typename T>
    inline T *attribute(CreateOption option = DontCreate);

    /**
     * Returns the attribute of the requested type or 0 if it is not available.
     */
    template <typename T>
    inline const T *attribute() const;

    /**
     * Removes and deletes the attribute of the requested type.
     */
    template <typename T>
    inline void removeAttribute();

    /**
     * Returns whether the entity has an attribute of the requested type.
     */
    template <typename T>
    inline bool hasAttribute() const;

    /**
     * Returns the url of the tag.
     */
    QUrl url() const;

    /**
     * Sets the unique @p identifier of the tag.
     */
    void setId(Id identifier);

    /**
     * Returns the unique identifier of the tag.
     */
    Id id() const;

    void setGid(const QByteArray &gid);
    QByteArray gid() const;

    void setRemoteId(const QByteArray &remoteId);
    QByteArray remoteId() const;

    void setType(const QByteArray &type);
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

    /**
     * Returns a GENERIC tag with the given name and a valid gid
     */
    static Tag genericTag(const QString &name);

private:
    bool checkAttribute(const Attribute *attr, const QByteArray &type) const;
    void markAttributeModified(const QByteArray &type);

    //@cond PRIVATE
    friend class TagModifyJob;
    friend class TagFetchJob;
    friend class ProtocolHelper;

    QSharedDataPointer<TagPrivate> d_ptr;
    //@endcond
};

AKONADICORE_EXPORT uint qHash(const Akonadi::Tag &);

template <typename T>
inline T *Tag::attribute(CreateOption option)
{
    const QByteArray type = T().type();
    if (hasAttribute(type)) {
        T *attr = dynamic_cast<T *>(attribute(type));
        if (checkAttribute(attr, type)) {
            return attr;
        }
    } else if (option == AddIfMissing) {
        T *attr = new T();
        addAttribute(attr);
        return attr;
    }

    return nullptr;
}

template <typename T>
inline const T *Tag::attribute() const
{
    const QByteArray type = T().type();
    if (hasAttribute(type)) {
        const T *attr = dynamic_cast<const T *>(attribute(type));
        if (checkAttribute(attr, type)) {
            return attr;
        }
    }

    return nullptr;
}

template <typename T>
inline void Tag::removeAttribute()
{
    const T dummy;
    removeAttribute(dummy.type());
}

template <typename T>
inline bool Tag::hasAttribute() const
{
    const T dummy;
    return hasAttribute(dummy.type());
}

} // namespace Akonadi

AKONADICORE_EXPORT QDebug &operator<<(QDebug &debug, const Akonadi::Tag &tag);

Q_DECLARE_METATYPE(Akonadi::Tag)
Q_DECLARE_METATYPE(Akonadi::Tag::List)
Q_DECLARE_METATYPE(QSet<Akonadi::Tag>)
Q_DECLARE_TYPEINFO(Akonadi::Tag, Q_MOVABLE_TYPE);

#endif
