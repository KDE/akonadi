/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tag.h"
#include "akonadicore_debug.h"
#include "tag_p.h"

#include "tagattribute.h"
#include <QUrlQuery>
#include <QUuid>

using namespace Akonadi;

const char Akonadi::Tag::PLAIN[] = "PLAIN";
const char Akonadi::Tag::GENERIC[] = "GENERIC";

uint Akonadi::qHash(const Tag &tag)
{
    return ::qHash(tag.id());
}

Tag::Tag()
    : d_ptr(new TagPrivate)
{
}

Tag::Tag(Tag::Id id)
    : d_ptr(new TagPrivate)
{
    d_ptr->id = id;
}

Tag::Tag(const QString &name)
    : d_ptr(new TagPrivate)
{
    d_ptr->gid = name.toUtf8();
    d_ptr->type = PLAIN;
}

Tag::Tag(const Tag &) = default;
Tag::Tag(Tag &&) noexcept = default;
Tag::~Tag() = default;

Tag &Tag::operator=(const Tag &) = default;
Tag &Tag::operator=(Tag &&) noexcept = default;

bool Tag::operator==(const Tag &other) const
{
    // Valid tags are equal if their IDs are equal
    if (isValid() && other.isValid()) {
        return d_ptr->id == other.d_ptr->id;
    }

    // Invalid tags are equal if their GIDs are non empty but equal
    if (!d_ptr->gid.isEmpty() || !other.d_ptr->gid.isEmpty()) {
        return d_ptr->gid == other.d_ptr->gid;
    }

    // Invalid tags are equal if both are invalid
    return !isValid() && !other.isValid();
}

bool Tag::operator!=(const Tag &other) const
{
    return !operator==(other);
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
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("tag"), QString::number(id()));

    QUrl url;
    url.setScheme(QStringLiteral("akonadi"));
    url.setQuery(query);
    return url;
}

void Tag::addAttribute(Attribute *attr)
{
    d_ptr->mAttributeStorage.addAttribute(attr);
}

void Tag::removeAttribute(const QByteArray &type)
{
    d_ptr->mAttributeStorage.removeAttribute(type);
}

bool Tag::hasAttribute(const QByteArray &type) const
{
    return d_ptr->mAttributeStorage.hasAttribute(type);
}

Attribute::List Tag::attributes() const
{
    return d_ptr->mAttributeStorage.attributes();
}

void Tag::clearAttributes()
{
    d_ptr->mAttributeStorage.clearAttributes();
}

const Attribute *Tag::attribute(const QByteArray &type) const
{
    return d_ptr->mAttributeStorage.attribute(type);
}

Attribute *Tag::attribute(const QByteArray &type)
{
    markAttributeModified(type);
    return d_ptr->mAttributeStorage.attribute(type);
}

void Tag::setId(Tag::Id identifier)
{
    d_ptr->id = identifier;
}

Tag::Id Tag::id() const
{
    return d_ptr->id;
}

void Tag::setGid(const QByteArray &gid)
{
    d_ptr->gid = gid;
}

QByteArray Tag::gid() const
{
    return d_ptr->gid;
}

void Tag::setRemoteId(const QByteArray &remoteId)
{
    d_ptr->remoteId = remoteId;
}

QByteArray Tag::remoteId() const
{
    return d_ptr->remoteId;
}

void Tag::setName(const QString &name)
{
    if (!name.isEmpty()) {
        auto *const attr = attribute<TagAttribute>(Tag::AddIfMissing);
        attr->setDisplayName(name);
    }
}

QString Tag::name() const
{
    const auto *const attr = attribute<TagAttribute>();
    const QString displayName = attr ? attr->displayName() : QString();
    return !displayName.isEmpty() ? displayName : QString::fromUtf8(d_ptr->gid);
}

void Tag::setParent(const Tag &parent)
{
    d_ptr->parent.reset(new Tag(parent));
}

Tag Tag::parent() const
{
    if (!d_ptr->parent) {
        return Tag();
    }
    return *d_ptr->parent;
}

void Tag::setType(const QByteArray &type)
{
    d_ptr->type = type;
}

QByteArray Tag::type() const
{
    return d_ptr->type;
}

bool Tag::isValid() const
{
    return d_ptr->id >= 0;
}

bool Tag::isImmutable() const
{
    return (d_ptr->type.isEmpty() || d_ptr->type == PLAIN);
}

QDebug operator<<(QDebug debug, const Tag &tag)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "Akonadi::Tag(ID " << tag.id() << ", GID " << tag.gid() << ", parent tag ID " << tag.parent().id() << ")";
    return debug;
}

Tag Tag::genericTag(const QString &name)
{
    Tag tag;
    tag.d_ptr->type = GENERIC;
    tag.d_ptr->gid = QUuid::createUuid().toByteArray().mid(1, 36);
    tag.setName(name);
    return tag;
}

bool Tag::checkAttribute(const Attribute *attr, const QByteArray &type) const
{
    if (attr) {
        return true;
    }
    qCWarning(AKONADICORE_LOG) << "Found attribute of unknown type" << type << ". Did you forget to call AttributeFactory::registerAttribute()?";
    return false;
}

void Tag::markAttributeModified(const QByteArray &type)
{
    d_ptr->mAttributeStorage.markAttributeModified(type);
}
