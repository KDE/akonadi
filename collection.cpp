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
#include "entity_p.h"
#include "entitydisplayattribute.h"

#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <KUrl>
#include <KGlobal>

using namespace Akonadi;

class CollectionRoot : public Collection
{
public:
    CollectionRoot()
        : Collection(0)
    {
        QStringList types;
        types << Collection::mimeType();
        setContentMimeTypes(types);

        // The root collection is read-only for the users
        Collection::Rights rights;
        rights |= Collection::ReadOnly;
        setRights(rights);
    }
};

K_GLOBAL_STATIC(CollectionRoot, s_root)

Collection::Collection()
    : Entity(new CollectionPrivate)
{
    Q_D(Collection);
    static int lastId = -1;
    d->mId = lastId--;
}

Collection::Collection(Id id)
    : Entity(new CollectionPrivate(id))
{
}

Collection::Collection(const Collection &other)
    : Entity(other)
{
}

Collection::~Collection()
{
}

QString Collection::name() const
{
    return d_func()->name;
}

QString Collection::displayName() const
{
    const EntityDisplayAttribute *const attr = attribute<EntityDisplayAttribute>();
    const QString displayName = attr ? attr->displayName() : QString();
    return !displayName.isEmpty() ? displayName : d_func()->name;
}

void Collection::setName(const QString &name)
{
    Q_D(Collection);
    d->name = name;
}

Collection::Rights Collection::rights() const
{
    CollectionRightsAttribute *attr = attribute<CollectionRightsAttribute>();
    if (attr) {
        return attr->rights();
    } else {
        return AllRights;
    }
}

void Collection::setRights(Rights rights)
{
    CollectionRightsAttribute *attr = attribute<CollectionRightsAttribute>(AddIfMissing);
    attr->setRights(rights);
}

QStringList Collection::contentMimeTypes() const
{
    return d_func()->contentTypes;
}

void Collection::setContentMimeTypes(const QStringList &types)
{
    Q_D(Collection);
    if (d->contentTypes != types) {
        d->contentTypes = types;
        d->contentTypesChanged = true;
    }
}

Collection::Id Collection::parent() const
{
    return parentCollection().id();
}

void Collection::setParent(Id parent)
{
    parentCollection().setId(parent);
}

void Collection::setParent(const Collection &collection)
{
    setParentCollection(collection);
}

QString Collection::parentRemoteId() const
{
    return parentCollection().remoteId();
}

void Collection::setParentRemoteId(const QString &remoteParent)
{
    parentCollection().setRemoteId(remoteParent);
}

KUrl Collection::url() const
{
    return url(UrlShort);
}

KUrl Collection::url(UrlType type) const
{
    KUrl url;
    url.setProtocol(QString::fromLatin1("akonadi"));
    url.addQueryItem(QLatin1String("collection"), QString::number(id()));

    if (type == UrlWithName) {
        url.addQueryItem(QLatin1String("name"), name());
    }

    return url;
}

Collection Collection::fromUrl(const KUrl &url)
{
    if (url.protocol() != QLatin1String("akonadi")) {
        return Collection();
    }

    const QString colStr = url.queryItem(QLatin1String("collection"));
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
    return QString::fromLatin1("inode/directory");
}

QString Akonadi::Collection::virtualMimeType()
{
    return QString::fromLatin1("application/x-vnd.akonadi.collection.virtual");
}

QString Collection::resource() const
{
    return d_func()->resource;
}

void Collection::setResource(const QString &resource)
{
    Q_D(Collection);
    d->resource = resource;
}

uint qHash(const Akonadi::Collection &collection)
{
    return qHash(collection.id());
}

QDebug operator <<(QDebug d, const Akonadi::Collection &collection)
{
    return d << "Collection ID:" << collection.id()
           << "   remote ID:" << collection.remoteId() << endl
           << "   name:" << collection.name() << endl
           << "   url:" << collection.url() << endl
           << "   parent:" << collection.parentCollection().id() << collection.parentCollection().remoteId() << endl
           << "   resource:" << collection.resource() << endl
           << "   rights:" << collection.rights() << endl
           << "   contents mime type:" << collection.contentMimeTypes() << endl
           << "   isVirtual:" << collection.isVirtual() << endl
           << "   " << collection.cachePolicy() << endl
           << "   " << collection.statistics();
}

CollectionStatistics Collection::statistics() const
{
    return d_func()->statistics;
}

void Collection::setStatistics(const CollectionStatistics &statistics)
{
    Q_D(Collection);
    d->statistics = statistics;
}

CachePolicy Collection::cachePolicy() const
{
    return d_func()->cachePolicy;
}

void Collection::setCachePolicy(const CachePolicy &cachePolicy)
{
    Q_D(Collection);
    d->cachePolicy = cachePolicy;
    d->cachePolicyChanged = true;
}

bool Collection::isVirtual() const
{
    return d_func()->isVirtual;
}

void Akonadi::Collection::setVirtual(bool isVirtual)
{
    Q_D(Collection);

    d->isVirtual = isVirtual;
}

void Collection::setEnabled(bool enabled)
{
    Q_D(Collection);

    d->enabledChanged = true;
    d->enabled = enabled;
}

bool Collection::enabled() const
{
    Q_D(const Collection);

    return d->enabled;
}

void Collection::setLocalListPreference(Collection::ListPurpose purpose, Collection::ListPreference preference)
{
    Q_D(Collection);

    switch(purpose) {
    case ListDisplay:
        d->displayPreference = preference;
        break;
    case ListSync:
        d->syncPreference = preference;
        break;
    case ListIndex:
        d->indexPreference = preference;
        break;
    }
    d->listPreferenceChanged = true;
}

Collection::ListPreference Collection::localListPreference(Collection::ListPurpose purpose) const
{
    Q_D(const Collection);

    switch(purpose) {
    case ListDisplay:
        return d->displayPreference;
    case ListSync:
        return d->syncPreference;
    case ListIndex:
        return d->indexPreference;
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

void Collection::setReferenced(bool referenced)
{
    Q_D(Collection);

    d->referencedChanged = true;
    d->referenced = referenced;
}

bool Collection::referenced() const
{
    Q_D(const Collection);

    return d->referenced;
}

void Collection::setKeepLocalChanges(const QSet<QByteArray> &parts)
{
    Q_D(Collection);

    d->keepLocalChanges = parts;
}

QSet<QByteArray> Collection::keepLocalChanges() const
{
    Q_D(const Collection);

    return d->keepLocalChanges;
}

AKONADI_DEFINE_PRIVATE(Akonadi::Collection)
