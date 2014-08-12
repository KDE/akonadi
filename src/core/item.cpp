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
#include "itemserializer_p.h"
#include <akonadi/private/protocol_p.h>

#include <QUrl>
#include <qdebug.h>

#include <QtCore/QStringList>
#include <QtCore/QReadWriteLock>

#include <algorithm>
#include <map>
#include <utility>

using namespace Akonadi;
using namespace boost;

namespace {

struct nodelete {
    template <typename T>
    void operator()(T *)
    {
    }
};

struct ByTypeId {
    typedef bool result_type;
    bool operator()(const boost::shared_ptr<PayloadBase> &lhs, const boost::shared_ptr<PayloadBase> &rhs) const
    {
        return strcmp(lhs->typeName(), rhs->typeName()) < 0 ;
    }
};

} // anon namespace

typedef QHash< QString, std::map< boost::shared_ptr<PayloadBase>, std::pair<int, int>, ByTypeId > > LegacyMap;
Q_GLOBAL_STATIC(LegacyMap, typeInfoToMetaTypeIdMap)
Q_GLOBAL_STATIC_WITH_ARGS(QReadWriteLock, legacyMapLock, (QReadWriteLock::Recursive))

void Item::addToLegacyMappingImpl(const QString &mimeType, int spid, int mtid, std::auto_ptr<PayloadBase> p)
{
    if (!p.get()) {
        return;
    }
    const boost::shared_ptr<PayloadBase> sp(p);
    const QWriteLocker locker(legacyMapLock());
    std::pair<int, int> &item = (*typeInfoToMetaTypeIdMap())[mimeType][sp];
    item.first = spid;
    item.second = mtid;
}

namespace {
class MyReadLocker {
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
    shared_ptr<T> makeUnlockingPointer(T *t)
    {
        if (t) {
            // the bind() doesn't throw, so if shared_ptr
            // construction line below, or anything else after it,
            // throws, we're unlocked. Mark us as such:
            locked = false;
            const shared_ptr<T> result(t, bind(&QReadWriteLock::unlock, rwl));
            // from now on, the shared_ptr is responsible for unlocking
            return result;
        } else {
            return shared_ptr<T>();
        }
    }
private:
    QReadWriteLock *const rwl;
    bool locked;
};
}

static shared_ptr<const std::pair<int, int> > lookupLegacyMapping(const QString &mimeType, PayloadBase *p)
{
    MyReadLocker locker(legacyMapLock());
    const LegacyMap::const_iterator hit = typeInfoToMetaTypeIdMap()->constFind(mimeType);
    if (hit == typeInfoToMetaTypeIdMap()->constEnd()) {
        return shared_ptr<const std::pair<int, int> >();
    }
    const boost::shared_ptr<PayloadBase> sp(p, nodelete());
    const LegacyMap::mapped_type::const_iterator it = hit->find(sp);
    if (it == hit->end()) {
        return shared_ptr<const std::pair<int, int> >();
    }

    return locker.makeUnlockingPointer(&it->second);
}

// Change to something != RFC822 as soon as the server supports it
const char *Item::FullPayload = "RFC822";

Item::Item()
    : Entity(new ItemPrivate)
{
}

Item::Item(Id id)
    : Entity(new ItemPrivate(id))
{
}

Item::Item(const QString &mimeType)
    : Entity(new ItemPrivate)
{
    d_func()->mMimeType = mimeType;
}

Item::Item(const Item &other)
    : Entity(other)
{
}

Item::~Item()
{
}

Item::Flags Item::flags() const
{
    return d_func()->mFlags;
}

void Item::setFlag(const QByteArray &name)
{
    Q_D(Item);
    d->mFlags.insert(name);
    if (!d->mFlagsOverwritten) {
        if (d->mDeletedFlags.contains(name)) {
            d->mDeletedFlags.remove(name);
        } else {
            d->mAddedFlags.insert(name);
        }
    }
}

void Item::clearFlag(const QByteArray &name)
{
    Q_D(Item);
    d->mFlags.remove(name);
    if (!d->mFlagsOverwritten) {
        if (d->mAddedFlags.contains(name)) {
            d->mAddedFlags.remove(name);
        } else {
            d->mDeletedFlags.insert(name);
        }
    }
}

void Item::setFlags(const Flags &flags)
{
    Q_D(Item);
    d->mFlags = flags;
    d->mFlagsOverwritten = true;
}

void Item::clearFlags()
{
    Q_D(Item);
    d->mFlags.clear();
    d->mFlagsOverwritten = true;
}

QDateTime Item::modificationTime() const
{
    return d_func()->mModificationTime;
}

void Item::setModificationTime(const QDateTime &datetime)
{
    d_func()->mModificationTime = datetime;
}

bool Item::hasFlag(const QByteArray &name) const
{
    return d_func()->mFlags.contains(name);
}

void Item::setTags(const Tag::List &list)
{
    Q_D(Item);
    d->mTags = list;
    d->mTagsOverwritten = true;
}

void Item::setTag(const Tag &tag)
{
    Q_D(Item);
    d->mTags << tag;
    if (!d->mTagsOverwritten) {
        if (d->mDeletedTags.contains(tag)) {
            d->mDeletedTags.removeOne(tag);
        } else {
            d->mAddedTags << tag ;
        }
    }
}

void Item::clearTags()
{
    Q_D(Item);
    d->mTags.clear();
    d->mTagsOverwritten = true;
}

void Item::clearTag(const Tag &tag)
{
    Q_D(Item);
    d->mTags.removeOne(tag);
    if (!d->mTagsOverwritten) {
        if (d->mAddedTags.contains(tag)) {
            d->mAddedTags.removeOne(tag);
        } else {
            d->mDeletedTags << tag;
        }
    }
}

bool Item::hasTag(const Tag &tag) const
{
    Q_D(const Item);
    return d->mTags.contains(tag);
}

Tag::List Item::tags() const
{
    Q_D(const Item);
    return d->mTags;
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
    ItemSerializer::deserialize(*this, FullPayload, data, 0, false);
}

void Item::clearPayload()
{
    d_func()->mClearPayload = true;
}

int Item::revision() const
{
    return d_func()->mRevision;
}

void Item::setRevision(int rev)
{
    d_func()->mRevision = rev;
}

Entity::Id Item::storageCollectionId() const
{
    return d_func()->mCollectionId;
}

void Item::setStorageCollectionId(Entity::Id collectionId)
{
    d_func()->mCollectionId = collectionId;
}

QString Item::mimeType() const
{
    return d_func()->mMimeType;
}

void Item::setSize(qint64 size)
{
    Q_D(Item);
    d->mSize = size;
    d->mSizeChanged = true;
}

qint64 Item::size() const
{
    return d_func()->mSize;
}

void Item::setMimeType(const QString &mimeType)
{
    d_func()->mMimeType = mimeType;
}

void Item::setGid(const QString &id)
{
    d_func()->mGid = id;
}

QString Item::gid() const
{
    return d_func()->mGid;
}

void Item::setVirtualReferences(const Collection::List &collections)
{
    d_func()->mVirtualReferences = collections;
}

Collection::List Item::virtualReferences() const
{
    return d_func()->mVirtualReferences;
}

bool Item::hasPayload() const
{
    return d_func()->hasMetaTypeId(-1);
}

QUrl Item::url(UrlType type) const
{
    QUrl url;
    url.setScheme(QString::fromLatin1("akonadi"));
    url.addQueryItem(QStringLiteral("item"), QString::number(id()));

    if (type == UrlWithMimeType) {
        url.addQueryItem(QStringLiteral("type"), mimeType());
    }

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

namespace {
class Dummy
{
};
}

Q_GLOBAL_STATIC(Payload<Dummy>, dummyPayload)

PayloadBase *Item::payloadBase() const
{
    Q_D(const Item);
    d->tryEnsureLegacyPayload();
    if (d->mLegacyPayload) {
        return d->mLegacyPayload.get();
    } else {
        return dummyPayload();
    }
}

void ItemPrivate::tryEnsureLegacyPayload() const
{
    if (!mLegacyPayload) {
        for (PayloadContainer::const_iterator it = mPayloads.begin(), end = mPayloads.end() ; it != end ; ++it) {
            if (lookupLegacyMapping(mMimeType, it->payload.get())) {
                mLegacyPayload = it->payload; // clones
            }
        }
    }
}

PayloadBase *Item::payloadBaseV2(int spid, int mtid) const
{
    return d_func()->payloadBaseImpl(spid, mtid);
}

namespace {
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
    ~ConversionGuard() {
        b = old;
    }
};
}

bool Item::ensureMetaTypeId(int mtid) const
{
    Q_D(const Item);
    // 0. Nothing there - nothing to convert from, either
    if (d->mPayloads.empty()) {
        return false;
    }

    // 1. Look whether we already have one:
    if (d->hasMetaTypeId(mtid)) {
        return true;
    }

    // recursion detection (shouldn't trigger, but does if the
    // serialiser plugins are acting funky):
    if (d->mConversionInProgress) {
        return false;
    }

    // 2. Try to create one by conversion from a different representation:
    try {
        const ConversionGuard guard(d->mConversionInProgress);
        Item converted = ItemSerializer::convert(*this, mtid);
        return d->movePayloadFrom(converted.d_func(), mtid);
    } catch (const std::exception &e) {
        qDebug() << "conversion threw:" << e.what();
        return false;
    } catch (...) {
        qDebug() << "conversion threw something not derived from std::exception: fix the program!";
        return false;
    }
}

static QString format_type(int spid, int mtid)
{
    return QString::fromLatin1("sp(%1)<%2>")
           .arg(spid).arg(QLatin1String(QMetaType::typeName(mtid)));
}

static QString format_types(const PayloadContainer &c)
{
    QStringList result;
    for (PayloadContainer::const_iterator it = c.begin(), end = c.end() ; it != end ; ++it) {
        result.push_back(format_type(it->sharedPointerId, it->metaTypeId));
    }
    return result.join(QStringLiteral(", "));
}

#if 0
QString Item::payloadExceptionText(int spid, int mtid) const
{
    Q_D(const Item);
    if (d->mPayloads.empty()) {
        return QStringLiteral("No payload set");
    } else {
        return QString::fromLatin1("Wrong payload type (requested: %1; present: %2")
               .arg(format_type(spid, mtid), format_types(d->mPayloads));
    }
}
#else
void Item::throwPayloadException(int spid, int mtid) const
{
    Q_D(const Item);
    if (d->mPayloads.empty()) {
        throw PayloadException("No payload set");
    } else {
        throw PayloadException(QString::fromLatin1("Wrong payload type (requested: %1; present: %2")
                               .arg(format_type(spid, mtid), format_types(d->mPayloads)));
    }
}
#endif

void Item::setPayloadBase(PayloadBase *p)
{
    d_func()->setLegacyPayloadBaseImpl(std::auto_ptr<PayloadBase>(p));
}

void ItemPrivate::setLegacyPayloadBaseImpl(std::auto_ptr<PayloadBase> p)
{
    if (const shared_ptr<const std::pair<int, int> > pair = lookupLegacyMapping(mMimeType, p.get())) {
        std::auto_ptr<PayloadBase> clone;
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

void Item::setPayloadBaseV2(int spid, int mtid, std::auto_ptr<PayloadBase> p)
{
    d_func()->setPayloadBaseImpl(spid, mtid, p, false);
}

void Item::addPayloadBaseVariant(int spid, int mtid, std::auto_ptr<PayloadBase> p) const
{
    d_func()->setPayloadBaseImpl(spid, mtid, p, true);
}

QSet<QByteArray> Item::cachedPayloadParts() const
{
    Q_D(const Item);
    return d->mCachedPayloadParts;
}

void Item::setCachedPayloadParts(const QSet< QByteArray > &cachedParts)
{
    Q_D(Item);
    d->mCachedPayloadParts = cachedParts;
}

QSet<QByteArray> Item::availablePayloadParts() const
{
    return ItemSerializer::availableParts(*this);
}

QVector<int> Item::availablePayloadMetaTypeIds() const
{
    QVector<int> result;
    Q_D(const Item);
    result.reserve(d->mPayloads.size());
    // Stable Insertion Sort - N is typically _very_ low (1 or 2).
    for (PayloadContainer::const_iterator it = d->mPayloads.begin(), end = d->mPayloads.end() ; it != end ; ++it) {
        result.insert(std::upper_bound(result.begin(), result.end(), it->metaTypeId), it->metaTypeId);
    }
    return result;
}

void Item::apply(const Item &other)
{
    if (mimeType() != other.mimeType() || id() != other.id()) {
        qDebug() << "mimeType() = " << mimeType() << "; other.mimeType() = " << other.mimeType();
        qDebug() << "id() = " << id() << "; other.id() = " << other.id();
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
    foreach (Attribute *attribute, other.attributes()) {
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
    d_func()->resetChangeLog();
}

AKONADI_DEFINE_PRIVATE(Item)
