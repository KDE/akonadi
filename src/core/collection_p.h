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
#include "attributestorage_p.h"

#include "qstringlist.h"

#include <QSharedData>

using namespace Akonadi;

/**
 * @internal
 */
class Akonadi::CollectionPrivate : public QSharedData
{
public:
    CollectionPrivate(Collection::Id id = -1)
        : QSharedData()
        , displayPreference(Collection::ListDefault)
        , syncPreference(Collection::ListDefault)
        , indexPreference(Collection::ListDefault)
        , listPreferenceChanged(false)
        , enabled(true)
        , enabledChanged(false)
        , contentTypesChanged(false)
        , cachePolicyChanged(false)
        , isVirtual(false)
        , mId(id)
        , mParent(nullptr)
    {
    }

    CollectionPrivate(const CollectionPrivate &other)
        : QSharedData(other)
        , mParent(nullptr)
    {
        mId = other.mId;
        mRemoteId = other.mRemoteId;
        mRemoteRevision = other.mRemoteRevision;
        mAttributeStorage = other.mAttributeStorage;
        if (other.mParent) {
            mParent = new Collection(*(other.mParent));
        }
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
        keepLocalChanges = other.keepLocalChanges;
    }

    ~CollectionPrivate()
    {
        delete mParent;
    }

    void resetChangeLog()
    {
        contentTypesChanged = false;
        cachePolicyChanged = false;
        enabledChanged = false;
        listPreferenceChanged = false;
        mAttributeStorage.resetChangeLog();
    }

    static Collection newRoot()
    {
        Collection rootCollection(0);
        rootCollection.setContentMimeTypes({ Collection::mimeType() });
        return rootCollection;
    }

    // Make use of the 4-bytes padding from QSharedData
    Collection::ListPreference displayPreference: 2;
    Collection::ListPreference syncPreference: 2;
    Collection::ListPreference indexPreference: 2;
    bool listPreferenceChanged: 1;
    bool enabled: 1;
    bool enabledChanged: 1;
    bool contentTypesChanged: 1;
    bool cachePolicyChanged: 1;
    bool isVirtual: 1;
    // 2 bytes padding here

    Collection::Id mId;
    QString mRemoteId;
    QString mRemoteRevision;
    mutable Collection *mParent;
    AttributeStorage mAttributeStorage;
    QString name;
    QString resource;
    CollectionStatistics statistics;
    QStringList contentTypes;
    static const Collection root;
    CachePolicy cachePolicy;
    QSet<QByteArray> keepLocalChanges;

};

#endif
