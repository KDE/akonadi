/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "item.h"
#include "akonadicore_debug.h"
#include "item_p.h"
#include "itemserializer_p.h"
#include "private/protocol_p.h"

#include <QUrl>
#include <QUrlQuery>

#include <QReadWriteLock>
#include <QScopedValueRollback>
#include <QStringList>

#include <algorithm>
#include <map>
#include <utility>

using namespace Akonadi;

Q_GLOBAL_STATIC(Akonadi::Collection, s_defaultParentCollection) // NOLINT(readability-redundant-member-init)

uint Akonadi::qHash(const Akonadi::Item &item)
{
    return ::qHash(item.id());
}

// Change to something != RFC822 as soon as the server supports it
const char Item::FullPayload[] = "RFC822";

Item::Item()
    : d_ptr(new ItemPrivate)
{
}

Item::Item(Id id)
    : d_ptr(new ItemPrivate(id))
{
}

Item::Item(const QString &mimeType)
    : d_ptr(new ItemPrivate)
{
    d_ptr->mMimeType = mimeType;
}

Item::Item(const Item &other) = default;

Item::Item(Item &&other) noexcept = default;

Item::~Item() = default;

void Item::setId(Item::Id identifier)
{
    d_ptr->mId = identifier;
}

Item::Id Item::id() const
{
    return d_ptr->mId;
}

void Item::setRemoteId(const QString &id)
{
    d_ptr->mRemoteId = id;
}

QString Item::remoteId() const
{
    return d_ptr->mRemoteId;
}

void Item::setRemoteRevision(const QString &revision)
{
    d_ptr->mRemoteRevision = revision;
}

QString Item::remoteRevision() const
{
    return d_ptr->mRemoteRevision;
}

bool Item::isValid() const
{
    return (d_ptr->mId >= 0);
}

bool Item::operator==(const Item &other) const
{
    // Invalid collections are the same, no matter what their internal ID is
    return (!isValid() && !other.isValid()) || (d_ptr->mId == other.d_ptr->mId);
}

bool Akonadi::Item::operator!=(const Item &other) const
{
    return (isValid() || other.isValid()) && (d_ptr->mId != other.d_ptr->mId);
}

Item &Item ::operator=(const Item &other)
{
    if (this != &other) {
        d_ptr = other.d_ptr;
    }

    return *this;
}

bool Akonadi::Item::operator<(const Item &other) const
{
    return d_ptr->mId < other.d_ptr->mId;
}

void Item::addAttribute(Attribute *attr)
{
    ItemChangeLog::instance()->attributeStorage(d_ptr).addAttribute(attr);
}

void Item::removeAttribute(const QByteArray &type)
{
    ItemChangeLog::instance()->attributeStorage(d_ptr).removeAttribute(type);
}

bool Item::hasAttribute(const QByteArray &type) const
{
    return ItemChangeLog::instance()->attributeStorage(d_ptr).hasAttribute(type);
}

Attribute::List Item::attributes() const
{
    return ItemChangeLog::instance()->attributeStorage(d_ptr).attributes();
}

void Akonadi::Item::clearAttributes()
{
    ItemChangeLog::instance()->attributeStorage(d_ptr).clearAttributes();
}

Attribute *Item::attribute(const QByteArray &type)
{
    return ItemChangeLog::instance()->attributeStorage(d_ptr).attribute(type);
}

const Attribute *Item::attribute(const QByteArray &type) const
{
    return ItemChangeLog::instance()->attributeStorage(d_ptr).attribute(type);
}

Collection &Item::parentCollection()
{
    if (!d_ptr->mParent) {
        d_ptr->mParent.reset(new Collection());
    }
    return *(d_ptr->mParent);
}

Collection Item::parentCollection() const
{
    if (!d_ptr->mParent) {
        return *(s_defaultParentCollection);
    } else {
        return *(d_ptr->mParent);
    }
}

void Item::setParentCollection(const Collection &parent)
{
    d_ptr->mParent.reset(new Collection(parent));
}

Item::Flags Item::flags() const
{
    return d_ptr->mFlags;
}

void Item::setFlag(const QByteArray &name)
{
    d_ptr->mFlags.insert(name);
    if (!d_ptr->mFlagsOverwritten) {
        Item::Flags &deletedFlags = ItemChangeLog::instance()->deletedFlags(d_ptr);
        auto iter = deletedFlags.find(name);
        if (iter != deletedFlags.end()) {
            deletedFlags.erase(iter);
        } else {
            ItemChangeLog::instance()->addedFlags(d_ptr).insert(name);
        }
    }
}

void Item::clearFlag(const QByteArray &name)
{
    d_ptr->mFlags.remove(name);
    if (!d_ptr->mFlagsOverwritten) {
        Item::Flags &addedFlags = ItemChangeLog::instance()->addedFlags(d_ptr);
        auto iter = addedFlags.find(name);
        if (iter != addedFlags.end()) {
            addedFlags.erase(iter);
        } else {
            ItemChangeLog::instance()->deletedFlags(d_ptr).insert(name);
        }
    }
}

void Item::setFlags(const Flags &flags)
{
    d_ptr->mFlags = flags;
    d_ptr->mFlagsOverwritten = true;
}

void Item::clearFlags()
{
    d_ptr->mFlags.clear();
    d_ptr->mFlagsOverwritten = true;
}

QDateTime Item::modificationTime() const
{
    return d_ptr->mModificationTime;
}

void Item::setModificationTime(const QDateTime &datetime)
{
    d_ptr->mModificationTime = datetime;
}

bool Item::hasFlag(const QByteArray &name) const
{
    return d_ptr->mFlags.contains(name);
}

void Item::setTags(const Tag::List &list)
{
    d_ptr->mTags = list;
    d_ptr->mTagsOverwritten = true;
}

void Item::setTag(const Tag &tag)
{
    d_ptr->mTags << tag;
    if (!d_ptr->mTagsOverwritten) {
        Tag::List &deletedTags = ItemChangeLog::instance()->deletedTags(d_ptr);
        if (deletedTags.contains(tag)) {
            deletedTags.removeOne(tag);
        } else {
            ItemChangeLog::instance()->addedTags(d_ptr).push_back(tag);
        }
    }
}

void Item::clearTags()
{
    d_ptr->mTags.clear();
    d_ptr->mTagsOverwritten = true;
}

void Item::clearTag(const Tag &tag)
{
    d_ptr->mTags.removeOne(tag);
    if (!d_ptr->mTagsOverwritten) {
        Tag::List &addedTags = ItemChangeLog::instance()->addedTags(d_ptr);
        if (addedTags.contains(tag)) {
            addedTags.removeOne(tag);
        } else {
            ItemChangeLog::instance()->deletedTags(d_ptr).push_back(tag);
        }
    }
}

bool Item::hasTag(const Tag &tag) const
{
    return d_ptr->mTags.contains(tag);
}

Tag::List Item::tags() const
{
    return d_ptr->mTags;
}

Relation::List Item::relations() const
{
    return d_ptr->mRelations;
}

QSet<QByteArray> Item::loadedPayloadParts() const
{
    return ItemSerializer::parts(*this);
}

QByteArray Item::payloadData() const
{
    int version = 0;
    QByteArray data;
    ItemSerializer::serialize(*this, FullPayload, data, version);
    return data;
}

void Item::setPayloadFromData(const QByteArray &data)
{
    ItemSerializer::deserialize(*this, FullPayload, data, 0, ItemSerializer::Internal);
}

void Item::clearPayload()
{
    d_ptr->mClearPayload = true;
}

int Item::revision() const
{
    return d_ptr->mRevision;
}

void Item::setRevision(int rev)
{
    d_ptr->mRevision = rev;
}

Collection::Id Item::storageCollectionId() const
{
    return d_ptr->mCollectionId;
}

void Item::setStorageCollectionId(Collection::Id collectionId)
{
    d_ptr->mCollectionId = collectionId;
}

QString Item::mimeType() const
{
    return d_ptr->mMimeType;
}

void Item::setSize(qint64 size)
{
    d_ptr->mSize = size;
    d_ptr->mSizeChanged = true;
}

qint64 Item::size() const
{
    return d_ptr->mSize;
}

void Item::setMimeType(const QString &mimeType)
{
    d_ptr->mMimeType = mimeType;
}

void Item::setGid(const QString &id)
{
    d_ptr->mGid = id;
}

QString Item::gid() const
{
    return d_ptr->mGid;
}

void Item::setVirtualReferences(const Collection::List &collections)
{
    d_ptr->mVirtualReferences = collections;
}

Collection::List Item::virtualReferences() const
{
    return d_ptr->mVirtualReferences;
}

bool Item::hasPayload() const
{
    return d_ptr->hasMetaTypeId(-1);
}

QUrl Item::url(UrlType type) const
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("item"), QString::number(id()));
    if (type == UrlWithMimeType) {
        query.addQueryItem(QStringLiteral("type"), mimeType());
    }

    QUrl url;
    url.setScheme(QStringLiteral("akonadi"));
    url.setQuery(query);
    return url;
}

Item Item::fromUrl(const QUrl &url)
{
    if (url.scheme() != QLatin1String("akonadi")) {
        return Item();
    }

    const QString itemStr = QUrlQuery(url).queryItemValue(QStringLiteral("item"));
    bool ok = false;
    Item::Id itemId = itemStr.toLongLong(&ok);
    if (!ok) {
        return Item();
    }

    return Item(itemId);
}

Internal::PayloadBase *Item::payloadBaseV2(int spid, int mtid) const
{
    return d_ptr->payloadBaseImpl(spid, mtid);
}

bool Item::ensureMetaTypeId(int mtid) const
{
    // 0. Nothing there - nothing to convert from, either
    if (d_ptr->mPayloads.empty()) {
        return false;
    }

    // 1. Look whether we already have one:
    if (d_ptr->hasMetaTypeId(mtid)) {
        return true;
    }

    // recursion detection (shouldn't trigger, but does if the
    // serialiser plugins are acting funky):
    if (d_ptr->mConversionInProgress) {
        return false;
    }

    // 2. Try to create one by conversion from a different representation:
    try {
        const QScopedValueRollback guard(d_ptr->mConversionInProgress, true);
        Item converted = ItemSerializer::convert(*this, mtid);
        return d_ptr->movePayloadFrom(converted.d_ptr, mtid);
    } catch (const std::exception &e) {
        qCWarning(AKONADICORE_LOG) << "Item payload conversion threw:" << e.what();
        return false;
    } catch (...) {
        qCCritical(AKONADICORE_LOG, "conversion threw something not derived from std::exception: fix the program!");
        return false;
    }
}

static QString format_type(int spid, int mtid)
{
    return QStringLiteral("sp(%1)<%2>").arg(spid).arg(QLatin1String(QMetaType(mtid).name()));
}

static QString format_types(const PayloadContainer &container)
{
    QStringList result;
    result.reserve(container.size());
    for (auto it = container.begin(), end = container.end(); it != end; ++it) {
        result.push_back(format_type(it->sharedPointerId, it->metaTypeId));
    }
    return result.join(QLatin1String(", "));
}

static QString format_reason(bool valid, Item::Id id)
{
    if (valid) {
        return QStringLiteral("itemId: %1").arg(id);
    } else {
        return QStringLiteral("Item is not valid");
    }
}

void Item::throwPayloadException(int spid, int mtid) const
{
    const auto reason = format_reason(isValid(), id());

    if (d_ptr->mPayloads.empty()) {
        qCDebug(AKONADICORE_LOG) << "Throwing PayloadException for Item" << id() << ": No payload set";
        throw PayloadException(QStringLiteral("No Item payload set (%1)").arg(reason));
    } else {
        const auto requestedType = format_type(spid, mtid);
        const auto presentType = format_types(d_ptr->mPayloads);
        qCDebug(AKONADICORE_LOG) << "Throwing PayloadException for Item" << id() << ": Wrong payload type (requested:" << requestedType
                                 << "; present: " << presentType << "), item mime type is" << mimeType();
        throw PayloadException(QStringLiteral("Wrong Item payload type (requested: %1; present: %2, %3)").arg(requestedType, presentType, reason));
    }
}

void Item::setPayloadBaseV2(int spid, int mtid, std::unique_ptr<Internal::PayloadBase> &p)
{
    d_ptr->setPayloadBaseImpl(spid, mtid, p, false);
}

void Item::addPayloadBaseVariant(int spid, int mtid, std::unique_ptr<Internal::PayloadBase> &p) const
{
    d_ptr->setPayloadBaseImpl(spid, mtid, p, true);
}

QSet<QByteArray> Item::cachedPayloadParts() const
{
    return d_ptr->mCachedPayloadParts;
}

void Item::setCachedPayloadParts(const QSet<QByteArray> &cachedParts)
{
    d_ptr->mCachedPayloadParts = cachedParts;
}

QSet<QByteArray> Item::availablePayloadParts() const
{
    return ItemSerializer::availableParts(*this);
}

QList<int> Item::availablePayloadMetaTypeIds() const
{
    QList<int> result;
    result.reserve(d_ptr->mPayloads.size());
    // Stable Insertion Sort - N is typically _very_ low (1 or 2).
    for (auto it = d_ptr->mPayloads.begin(), end = d_ptr->mPayloads.end(); it != end; ++it) {
        result.insert(std::upper_bound(result.begin(), result.end(), it->metaTypeId), it->metaTypeId);
    }
    return result;
}

void Item::setPayloadPath(const QString &filePath)
{
    // Load payload from the external file, so that it's accessible via
    // Item::payload(). It internally calls setPayload(), which will clear
    // mPayloadPath, so we call it afterwards
    ItemSerializer::deserialize(*this, "RFC822", filePath.toUtf8(), 0, ItemSerializer::Foreign);
    d_ptr->mPayloadPath = filePath;
}

QString Item::payloadPath() const
{
    return d_ptr->mPayloadPath;
}

void Item::apply(const Item &other)
{
    if (mimeType() != other.mimeType() || id() != other.id()) {
        qCDebug(AKONADICORE_LOG) << "mimeType() = " << mimeType() << "; other.mimeType() = " << other.mimeType();
        qCDebug(AKONADICORE_LOG) << "id() = " << id() << "; other.id() = " << other.id();
        Q_ASSERT_X(false, "Item::apply", "mimetype or id mismatch");
    }

    setRemoteId(other.remoteId());
    setRevision(other.revision());
    setRemoteRevision(other.remoteRevision());
    setFlags(other.flags());
    setTags(other.tags());
    setModificationTime(other.modificationTime());
    setSize(other.size());
    setParentCollection(other.parentCollection());
    setStorageCollectionId(other.storageCollectionId());

    ItemChangeLog *changelog = ItemChangeLog::instance();
    changelog->attributeStorage(d_ptr) = changelog->attributeStorage(other.d_ptr);

    ItemSerializer::apply(*this, other);
    d_ptr->resetChangeLog();

    // Must happen after payload update
    d_ptr->mPayloadPath = other.payloadPath();
}
