/*
    Copyright (c) 2006 - 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTION_P_H
#define AKONADI_COLLECTION_P_H

#include "collection.h"
#include "cachepolicy.h"
#include "collectionstatistics.h"
#include "entity_p.h"

#include "qstringlist.h"

using namespace Akonadi;

/**
 * @internal
 */
class Akonadi::CollectionPrivate : public EntityPrivate
{
public:
    CollectionPrivate(Collection::Id id = -1)
        : EntityPrivate(id)
        , contentTypesChanged(false)
        , cachePolicyChanged(false)
        , isVirtual(false)
        , enabled(true)
        , enabledChanged(false)
        , displayPreference(Collection::ListDefault)
        , syncPreference(Collection::ListDefault)
        , indexPreference(Collection::ListDefault)
        , listPreferenceChanged(false)
        , referenced(false)
        , referencedChanged(false)
    {
    }

    CollectionPrivate(const CollectionPrivate &other)
        : EntityPrivate(other)
    {
        name = other.name;
        resource = other.resource;
        statistics = other.statistics;
        contentTypes = other.contentTypes;
        cachePolicy = other.cachePolicy;
        contentTypesChanged = other.contentTypesChanged;
        cachePolicyChanged = other.cachePolicyChanged;
        isVirtual = other.isVirtual;
        enabled = other.enabled;
        enabledChanged = other.enabledChanged;
        displayPreference = other.displayPreference;
        syncPreference = other.syncPreference;
        indexPreference = other.indexPreference;
        listPreferenceChanged = other.listPreferenceChanged;
        referenced = other.referenced;
        referencedChanged = other.referencedChanged;
        keepLocalChanges = other.keepLocalChanges;
    }

    ~CollectionPrivate()
    {
    }

    CollectionPrivate *clone() const Q_DECL_OVERRIDE
    {
        return new CollectionPrivate(*this);
    }

    void resetChangeLog() Q_DECL_OVERRIDE
    {
        contentTypesChanged = false;
        cachePolicyChanged = false;
        enabledChanged = false;
        listPreferenceChanged = false;
        referencedChanged = false;
        EntityPrivate::resetChangeLog();
    }

    static Collection newRoot()
    {
        Collection rootCollection(0);
        QStringList types;
        types << Collection::mimeType();
        rootCollection.setContentMimeTypes(types);
        return rootCollection;
    }

    QString name;
    QString resource;
    CollectionStatistics statistics;
    QStringList contentTypes;
    static const Collection root;
    CachePolicy cachePolicy;
    bool contentTypesChanged: 1;
    bool cachePolicyChanged: 1;
    bool isVirtual: 1;
    bool enabled;
    bool enabledChanged;
    Collection::ListPreference displayPreference;
    Collection::ListPreference syncPreference;
    Collection::ListPreference indexPreference;
    bool listPreferenceChanged;
    bool referenced;
    bool referencedChanged;
    QSet<QByteArray> keepLocalChanges;
};

#endif
