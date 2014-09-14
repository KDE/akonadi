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

#include "tag.h"
#include "tagattribute.h"

using namespace Akonadi;

const char *const Akonadi::Tag::PLAIN = "PLAIN";

struct Akonadi::Tag::Private {
    Private()
        : id(-1)
    {}

    ~Private()
    {}

    Id id;
    QByteArray gid;
    QByteArray remoteId;
    QScopedPointer<Tag> parent;
    QByteArray type;
};

Tag::Tag()
    : AttributeEntity()
    , d(new Private)
{

}

Tag::Tag(Tag::Id id)
    : AttributeEntity()
    , d(new Private)
{
    d->id = id;
}

Tag::Tag(const QString &name)
    : AttributeEntity()
    , d(new Private)
{
    d->gid = name.toUtf8();
    setName(name);
    d->type = PLAIN;
}

Tag::Tag(const Tag &other)
    : AttributeEntity()
    , d(new Private)
{
    operator=(other);
}

Tag::~Tag()
{
}

Tag &Tag::operator=(const Tag &other)
{
    d->id = other.d->id;
    d->gid = other.d->gid;
    d->remoteId = other.d->remoteId;
    d->type = other.d->type;
    if (other.d->parent) {
        d->parent.reset(new Tag(*other.d->parent));
    }
    AttributeEntity::operator=(other);
    return *this;
}

AttributeEntity &Tag::operator=(const AttributeEntity &other)
{
    return operator=(*static_cast<const Tag *>(&other));
}

bool Tag::operator==(const Tag &other) const
{
    if (isValid() && other.isValid()) {
        return d->id == other.d->id;
    }
    return d->gid == other.d->gid;
}

Tag Tag::fromUrl(const QUrl &url)
{
    if (url.scheme() != QLatin1String("akonadi")) {
        return Tag();
    }

    const QString tagStr = QUrlQuery(url).queryItemValue(QStringLiteral("tag"));
    bool ok = false;
    Tag::Id itemId = tagStr.toLongLong(&ok);
    if (!ok) {
        return Tag();
    }

    return Tag(itemId);
}

QUrl Tag::url() const
{
    QUrl url;
    url.setScheme(QString::fromLatin1("akonadi"));
    url.addQueryItem(QStringLiteral("tag"), QString::number(id()));
    return url;
}

void Tag::setId(Tag::Id identifier)
{
    d->id = identifier;
}

Tag::Id Tag::id() const
{
    return d->id;
}

void Tag::setGid(const QByteArray &gid) const
{
    d->gid = gid;
}

QByteArray Tag::gid() const
{
    return d->gid;
}

void Tag::setRemoteId(const QByteArray &remoteId) const
{
    d->remoteId = remoteId;
}

QByteArray Tag::remoteId() const
{
    return d->remoteId;
}

void Tag::setName(const QString &name)
{
    if (!name.isEmpty()) {
        TagAttribute *const attr = attribute<TagAttribute>(AttributeEntity::AddIfMissing);
        attr->setDisplayName(name);
    }
}

QString Tag::name() const
{
    const TagAttribute *const attr = attribute<TagAttribute>();
    const QString displayName = attr ? attr->displayName() : QString();
    return !displayName.isEmpty() ? displayName : QString::fromUtf8(d->gid);
}

void Tag::setParent(const Tag &parent)
{
    if (parent.isValid()) {
        d->parent.reset(new Tag(parent));
    }
}

Tag Tag::parent() const
{
    if (!d->parent) {
        return Tag();
    }
    return *d->parent;
}

void Tag::setType(const QByteArray &type) const
{
    d->type = type;
}

QByteArray Tag::type() const
{
    return d->type;
}

bool Tag::isValid() const
{
    return d->id >= 0;
}

bool Tag::isImmutable() const
{
    return (d->type.isEmpty() || d->type == PLAIN);
}

uint qHash(const Tag &tag)
{
    return qHash(tag.id());
}

QDebug &operator<<(QDebug &debug, const Tag &tag)
{
    debug << "Akonadi::Tag( ID " << tag.id() << ", GID " << tag.gid() << ", parent" << tag.parent().id() << ")";
    return debug;
}
