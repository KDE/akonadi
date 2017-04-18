/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "item.h"
#include "item_p.h"
#include "akonadicore_debug.h"
#include "itemserializer_p.h"
#include "private/protocol_p.h"


#include <QUrl>
#include <QUrlQuery>

#include <QStringList>
#include <QReadWriteLock>

#include <algorithm>
#include <map>
#include <utility>

using namespace Akonadi;

Q_GLOBAL_STATIC(Akonadi::Collection, s_defaultParentCollection)

uint Akonadi::qHash(const Akonadi::Item &item)
{
    return ::qHash(item.id());
}

namespace
{

struct ByTypeId {
    typedef bool result_type;
    bool operator()(const std::shared_ptr<Internal::PayloadBase> &lhs,
                    const std::shared_ptr<Internal::PayloadBase> &rhs) const
    {
        return strcmp(lhs->typeName(), rhs->typeName()) < 0;
    }
};

} // anon namespace

typedef QHash<QString, std::map<std::shared_ptr<Internal::PayloadBase>, std::pair<int, int>, ByTypeId>> LegacyMap;
Q_GLOBAL_STATIC(LegacyMap, typeInfoToMetaTypeIdMap)
Q_GLOBAL_STATIC_WITH_ARGS(QReadWriteLock, legacyMapLock, (QReadWriteLock::Recursive))

void Item::addToLegacyMappingImpl(const QString &mimeType, int spid, int mtid,
                                  std::unique_ptr<Internal::PayloadBase> &p)
{
    if (!p.get()) {
        return;
    }
    const std::shared_ptr<Internal::PayloadBase> sp(p.release());
    const QWriteLocker locker(legacyMapLock());
    std::pair<int, int> &item = (*typeInfoToMetaTypeIdMap())[mimeType][sp];
    item.first = spid;
    item.second = mtid;
}

namespace
{
class MyReadLocker
{
public:
    explicit MyReadLocker(QReadWriteLock *rwl)
        : rwl(rwl)
        , locked(false)
    {
        if (rwl) {
            rwl->lockForRead();
        }
        locked = true;
    }

    ~MyReadLocker()
    {
        if (rwl && locked) {
            rwl->unlock();
        }
    }

    template <typename T>
    std::shared_ptr<T> makeUnlockingPointer(T *t)
    {
        if (t) {
            // the bind() doesn't throw, so if shared_ptr
            // construction line below, or anything else after it,
            // throws, we're unlocked. Mark us as such:
            locked = false;
            const std::shared_ptr<T> result(t, [&](const void *) {
                rwl->unlock();
            });
            // from now on, the shared_ptr is responsible for unlocking
            return result;
        } else {
            return std::shared_ptr<T>();
        }
    }
private:
    Q_DISABLE_COPY(MyReadLocker)
    QReadWriteLock *const rwl;
    bool locked;
};
}

static std::shared_ptr<const std::pair<int, int>> lookupLegacyMapping(const QString &mimeType,
        Internal::PayloadBase *p)
{
    MyReadLocker locker(legacyMapLock());
    const LegacyMap::const_iterator hit = typeInfoToMetaTypeIdMap()->constFind(mimeType);
    if (hit == typeInfoToMetaTypeIdMap()->constEnd()) {
        return std::shared_ptr<const std::pair<int, int>>();
    }
    const std::shared_ptr<Internal::PayloadBase> sp(p, [ = ](Internal::PayloadBase *) {
        /*noop*/
    });
    const LegacyMap::mapped_type::const_iterator it = hit->find(sp);
    if (it == hit->end()) {
        return std::shared_ptr<const std::pair<int, int>>();
    }

    return locker.makeUnlockingPointer(&it->second);
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

Item::Item(const Item &other)
    : d_ptr(other.d_ptr)
{
}

Item::~Item()
{
}

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
    Q_ASSERT(attr);
    Attribute *existing = d_ptr->mAttributes.value(attr->type());
    if (existing) {
        if (attr == existing) {
            return;
        }
        d_ptr->mAttributes.remove(attr->type());
        delete existing;
    }
    d_ptr->mAttributes.insert(attr->type(), attr);
    ItemChangeLog::instance()->deletedAttributes(d_ptr).remove(attr->type());
}

void Item::removeAttribute(const QByteArray &type)
{
    ItemChangeLog::instance()->deletedAttributes(d_ptr).insert(type);
    delete d_ptr->mAttributes.take(type);
}

bool Item::hasAttribute(const QByteArray &type) const
{
    return d_ptr->mAttributes.contains(type);
}

Attribute::List Item::attributes() const
{
    return d_ptr->mAttributes.values();
}

void Akonadi::Item::clearAttributes()
{
    ItemChangeLog *changelog = ItemChangeLog::instance();
    QSet<QByteArray> &deletedAttributes = changelog->deletedAttributes(d_ptr);
    for (Attribute *attr : qAsConst(d_ptr->mAttributes)) {
        deletedAttributes.insert(attr->type());
        delete attr;
    }
    d_ptr->mAttributes.clear();
}

Attribute *Item::attribute(const QByteArray &type) const
{
    return d_ptr->mAttributes.value(type);
}

Collection &Item::parentCollection()
{
    if (!d_ptr->mParent) {
        d_ptr->mParent = new Collection();
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
    delete d_ptr->mParent;
    d_ptr->mParent = new Collection(parent);
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

namespace
{
class Dummy
{
};
}

Q_GLOBAL_STATIC(Internal::Payload<Dummy>, dummyPayload)

Internal::PayloadBase *Item::payloadBase() const
{
    d_ptr->tryEnsureLegacyPayload();
    if (d_ptr->mLegacyPayload) {
        return d_ptr->mLegacyPayload.get();
    } else {
        return dummyPayload();
    }
}

void ItemPrivate::tryEnsureLegacyPayload() const
{
    if (!mLegacyPayload) {
        for (PayloadContainer::const_iterator it = mPayloads.begin(), end = mPayloads.end(); it != end; ++it) {
            if (lookupLegacyMapping(mMimeType, it->payload.get())) {
                mLegacyPayload = it->payload; // clones
            }
        }
    }
}

Internal::PayloadBase *Item::payloadBaseV2(int spid, int mtid) const
{
    return d_ptr->payloadBaseImpl(spid, mtid);
}

namespace
{
class ConversionGuard
{
    const bool old;
    bool &b;
public:
    explicit ConversionGuard(bool &b)
        : old(b)
        , b(b)
    {
        b = true;
    }
    ~ConversionGuard()
    {
        b = old;
    }
private:
    Q_DISABLE_COPY(ConversionGuard)
};
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
        const ConversionGuard guard(d_ptr->mConversionInProgress);
        Item converted = ItemSerializer::convert(*this, mtid);
        return d_ptr->movePayloadFrom(converted.d_ptr, mtid);
    } catch (const std::exception &e) {
        qCDebug(AKONADICORE_LOG) << "conversion threw:" << e.what();
        return false;
    } catch (...) {
        qCDebug(AKONADICORE_LOG) << "conversion threw something not derived from std::exception: fix the program!";
        return false;
    }
}

static QString format_type(int spid, int mtid)
{
    return QStringLiteral("sp(%1)<%2>")
           .arg(spid).arg(QLatin1String(QMetaType::typeName(mtid)));
}

static QString format_types(const PayloadContainer &c)
{
    QStringList result;
    result.reserve(c.size());
    for (PayloadContainer::const_iterator it = c.begin(), end = c.end(); it != end; ++it) {
        result.push_back(format_type(it->sharedPointerId, it->metaTypeId));
    }
    return result.join(QStringLiteral(", "));
}

#if 0
QString Item::payloadExceptionText(int spid, int mtid) const
{
    if (d_ptr->mPayloads.empty()) {
        return QStringLiteral("No payload set");
    } else {
        return QStringLiteral("Wrong payload type (requested: %1; present: %2")
               .arg(format_type(spid, mtid), format_types(d_ptr->mPayloads));
    }
}
#else
void Item::throwPayloadException(int spid, int mtid) const
{
    if (d_ptr->mPayloads.empty()) {
        throw PayloadException("No payload set");
    } else {
        throw PayloadException(QStringLiteral("Wrong payload type (requested: %1; present: %2")
                               .arg(format_type(spid, mtid), format_types(d_ptr->mPayloads)));
    }
}
#endif

void Item::setPayloadBase(Internal::PayloadBase *p)
{
    d_ptr->setLegacyPayloadBaseImpl(std::unique_ptr<Internal::PayloadBase>(p));
}

void ItemPrivate::setLegacyPayloadBaseImpl(std::unique_ptr<Internal::PayloadBase> p)
{
    if (const std::shared_ptr<const std::pair<int, int>> pair = lookupLegacyMapping(mMimeType, p.get())) {
        std::unique_ptr<Internal::PayloadBase> clone;
        if (p.get()) {
            clone.reset(p->clone());
        }
        setPayloadBaseImpl(pair->first, pair->second, p, false);
        mLegacyPayload.reset(clone.release());
    } else {
        mPayloads.clear();
        mLegacyPayload.reset(p.release());
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

QVector<int> Item::availablePayloadMetaTypeIds() const
{
    QVector<int> result;
    result.reserve(d_ptr->mPayloads.size());
    // Stable Insertion Sort - N is typically _very_ low (1 or 2).
    for (PayloadContainer::const_iterator it = d_ptr->mPayloads.begin(), end = d_ptr->mPayloads.end(); it != end; ++it) {
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
        Q_ASSERT_X(false, "Item::apply", "mimetype or id missmatch");
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

    QList<QByteArray> attrs;
    attrs.reserve(other.attributes().count());
    const Akonadi::Attribute::List lstAttrs = other.attributes();
    for (Attribute *attribute : lstAttrs) {
        addAttribute(attribute->clone());
        attrs.append(attribute->type());
    }

    QMutableHashIterator<QByteArray, Attribute *> it(d_ptr->mAttributes);
    while (it.hasNext()) {
        it.next();
        if (!attrs.contains(it.key())) {
            delete it.value();
            it.remove();
        }
    }

    ItemSerializer::apply(*this, other);
    d_ptr->resetChangeLog();

    // Must happen after payload update
    d_ptr->mPayloadPath = other.payloadPath();
}
