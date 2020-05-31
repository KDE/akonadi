/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "collection.h"
#include "collection_p.h"

#include "attributefactory.h"
#include "cachepolicy.h"
#include "collectionrightsattribute_p.h"
#include "collectionstatistics.h"
#include "entitydisplayattribute.h"


#include <QDebug>
#include <QHash>
#include <QString>
#include <QStringList>

#include <QUrl>
#include <QUrlQuery>

using namespace Akonadi;

Q_GLOBAL_STATIC(Akonadi::Collection, s_defaultParentCollection)

uint Akonadi::qHash(const Akonadi::Collection &collection)
{
    return ::qHash(collection.id());
}

/**
 * Helper method for assignment operator and copy constructor.
 */
static void assignCollectionPrivate(QSharedDataPointer<CollectionPrivate> &one,
                                    const QSharedDataPointer<CollectionPrivate> &other)
{
    // We can't simply do one = other here, we have to use a temp.
    // Otherwise ProtocolHelperTest::testParentCollectionAfterCollectionParsing()
    // will break.
    //
    // The reason are assignments like
    //   col = col.parentCollection()
    //
    // Here, parentCollection() actually returns a reference to a pointer owned
    // by col. So when col (or rather, it's private class) is deleted, the pointer
    // to the parent collection and therefore the reference becomes invalid.
    //
    // With a single-line assignment here, the parent collection would be deleted
    // before it is assigned, and therefore the resulting object would point to
    // uninitalized memory.
    QSharedDataPointer<CollectionPrivate> temp = other;
    one = temp;
}

class CollectionRoot : public Collection
{
public:
    CollectionRoot()
        : Collection(0)
    {
        setContentMimeTypes({ Collection::mimeType() });

        // The root collection is read-only for the users
        setRights(Collection::ReadOnly);
    }
};

Q_GLOBAL_STATIC(CollectionRoot, s_root)

Collection::Collection()
    : d_ptr(new CollectionPrivate)
{
    static int lastId = -1;
    d_ptr->mId = lastId--;
}

Collection::Collection(Id id)
    : d_ptr(new CollectionPrivate(id))
{
}

Collection::Collection(const Collection &other)
{
    assignCollectionPrivate(d_ptr, other.d_ptr);
}

Collection::Collection(Collection &&) = default;

Collection::~Collection() = default;

void Collection::setId(Collection::Id identifier)
{
    d_ptr->mId = identifier;
}

Collection::Id Collection::id() const
{
    return d_ptr->mId;
}

void Collection::setRemoteId(const QString &id)
{
    d_ptr->mRemoteId = id;
}

QString Collection::remoteId() const
{
    return d_ptr->mRemoteId;
}

void Collection::setRemoteRevision(const QString &revision)
{
    d_ptr->mRemoteRevision = revision;
}

QString Collection::remoteRevision() const
{
    return d_ptr->mRemoteRevision;
}

bool Collection::isValid() const
{
    return (d_ptr->mId >= 0);
}

bool Collection::operator==(const Collection &other) const
{
    // Invalid collections are the same, no matter what their internal ID is
    return (!isValid() && !other.isValid()) || (d_ptr->mId == other.d_ptr->mId);
}

bool Akonadi::Collection::operator!=(const Collection &other) const
{
    return (isValid() || other.isValid()) && (d_ptr->mId != other.d_ptr->mId);
}

Collection &Collection ::operator=(const Collection &other)
{
    if (this != &other) {
        assignCollectionPrivate(d_ptr, other.d_ptr);
    }

    return *this;
}

bool Akonadi::Collection::operator<(const Collection &other) const
{
    return d_ptr->mId < other.d_ptr->mId;
}

void Collection::addAttribute(Attribute *attr)
{
    d_ptr->mAttributeStorage.addAttribute(attr);
}

void Collection::removeAttribute(const QByteArray &type)
{
    d_ptr->mAttributeStorage.removeAttribute(type);
}

bool Collection::hasAttribute(const QByteArray &type) const
{
    return d_ptr->mAttributeStorage.hasAttribute(type);
}

Attribute::List Collection::attributes() const
{
    return d_ptr->mAttributeStorage.attributes();
}

void Akonadi::Collection::clearAttributes()
{
    d_ptr->mAttributeStorage.clearAttributes();
}

Attribute *Collection::attribute(const QByteArray &type)
{
    markAttributeModified(type);
    return d_ptr->mAttributeStorage.attribute(type);
}

const Attribute *Collection::attribute(const QByteArray &type) const
{
    return d_ptr->mAttributeStorage.attribute(type);
}

Collection &Collection::parentCollection()
{
    if (!d_ptr->mParent) {
        d_ptr->mParent.reset(new Collection());
    }
    return *d_ptr->mParent.get();
}

Collection Collection::parentCollection() const
{
    if (!d_ptr->mParent) {
        return *(s_defaultParentCollection);
    } else {
        return *d_ptr->mParent.get();
    }
}

void Collection::setParentCollection(const Collection &parent)
{
    d_ptr->mParent.reset(new Collection(parent));
}

QString Collection::name() const
{
    return d_ptr->name;
}

QString Collection::displayName() const
{
    const EntityDisplayAttribute *const attr = attribute<EntityDisplayAttribute>();
    const QString displayName = attr ? attr->displayName() : QString();
    return !displayName.isEmpty() ? displayName : d_ptr->name;
}

void Collection::setName(const QString &name)
{
    d_ptr->name = name;
}

Collection::Rights Collection::rights() const
{
    if (const auto attr = attribute<CollectionRightsAttribute>()) {
        return attr->rights();
    } else {
        return AllRights;
    }
}

void Collection::setRights(Rights rights)
{
    attribute<CollectionRightsAttribute>(AddIfMissing)->setRights(rights);
}

QStringList Collection::contentMimeTypes() const
{
    return d_ptr->contentTypes;
}

void Collection::setContentMimeTypes(const QStringList &types)
{
    if (d_ptr->contentTypes != types) {
        d_ptr->contentTypes = types;
        d_ptr->contentTypesChanged = true;
    }
}

QUrl Collection::url(UrlType type) const
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("collection"), QString::number(id()));
    if (type == UrlWithName) {
        query.addQueryItem(QStringLiteral("name"), name());
    }

    QUrl url;
    url.setScheme(QStringLiteral("akonadi"));
    url.setQuery(query);
    return url;
}

Collection Collection::fromUrl(const QUrl &url)
{
    if (url.scheme() != QLatin1String("akonadi")) {
        return Collection();
    }

    const QString colStr = QUrlQuery(url).queryItemValue(QStringLiteral("collection"));
    bool ok = false;
    Collection::Id colId = colStr.toLongLong(&ok);
    if (!ok) {
        return Collection();
    }

    if (colId == 0) {
        return Collection::root();
    }

    return Collection(colId);
}

Collection Collection::root()
{
    return *s_root;
}

QString Collection::mimeType()
{
    return QStringLiteral("inode/directory");
}

QString Akonadi::Collection::virtualMimeType()
{
    return QStringLiteral("application/x-vnd.akonadi.collection.virtual");
}

QString Collection::resource() const
{
    return d_ptr->resource;
}

void Collection::setResource(const QString &resource)
{
    d_ptr->resource = resource;
}

QDebug operator <<(QDebug d, const Akonadi::Collection &collection)
{
    return d << "Collection ID:" << collection.id()
           << "   remote ID:" << collection.remoteId() << '\n'
           << "   name:" << collection.name() << '\n'
           << "   url:" << collection.url() << '\n'
           << "   parent:" << collection.parentCollection().id() << collection.parentCollection().remoteId() << '\n'
           << "   resource:" << collection.resource() << '\n'
           << "   rights:" << collection.rights() << '\n'
           << "   contents mime type:" << collection.contentMimeTypes() << '\n'
           << "   isVirtual:" << collection.isVirtual() << '\n'
           << "   " << collection.cachePolicy() << '\n'
           << "   " << collection.statistics();
}

CollectionStatistics Collection::statistics() const
{
    return d_ptr->statistics;
}

void Collection::setStatistics(const CollectionStatistics &statistics)
{
    d_ptr->statistics = statistics;
}

CachePolicy Collection::cachePolicy() const
{
    return d_ptr->cachePolicy;
}

void Collection::setCachePolicy(const CachePolicy &cachePolicy)
{
    d_ptr->cachePolicy = cachePolicy;
    d_ptr->cachePolicyChanged = true;
}

bool Collection::isVirtual() const
{
    return d_ptr->isVirtual;
}

void Akonadi::Collection::setVirtual(bool isVirtual)
{
    d_ptr->isVirtual = isVirtual;
}

void Collection::setEnabled(bool enabled)
{
    d_ptr->enabledChanged = true;
    d_ptr->enabled = enabled;
}

bool Collection::enabled() const
{
    return d_ptr->enabled;
}

void Collection::setLocalListPreference(Collection::ListPurpose purpose, Collection::ListPreference preference)
{
    switch (purpose) {
    case ListDisplay:
        d_ptr->displayPreference = preference;
        break;
    case ListSync:
        d_ptr->syncPreference = preference;
        break;
    case ListIndex:
        d_ptr->indexPreference = preference;
        break;
    }
    d_ptr->listPreferenceChanged = true;
}

Collection::ListPreference Collection::localListPreference(Collection::ListPurpose purpose) const
{
    switch (purpose) {
    case ListDisplay:
        return d_ptr->displayPreference;
    case ListSync:
        return d_ptr->syncPreference;
    case ListIndex:
        return d_ptr->indexPreference;
    }
    return ListDefault;
}

bool Collection::shouldList(Collection::ListPurpose purpose) const
{
    if (localListPreference(purpose) == ListDefault) {
        return enabled();
    }
    return (localListPreference(purpose) == ListEnabled);
}

void Collection::setShouldList(ListPurpose purpose, bool list)
{
    if (localListPreference(purpose) == ListDefault) {
        setEnabled(list);
    } else {
        setLocalListPreference(purpose, list ? ListEnabled : ListDisabled);
    }
}

void Collection::setKeepLocalChanges(const QSet<QByteArray> &parts)
{
    d_ptr->keepLocalChanges = parts;
}

QSet<QByteArray> Collection::keepLocalChanges() const
{
    return d_ptr->keepLocalChanges;
}

void Collection::markAttributeModified(const QByteArray &type)
{
    d_ptr->mAttributeStorage.markAttributeModified(type);
}
