/*
    SPDX-FileCopyrightText: 2006-2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "attributestorage_p.h"
#include "cachepolicy.h"
#include "collection.h"
#include "collectionstatistics.h"

#include <QStringList>

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
    {
    }

    CollectionPrivate(const CollectionPrivate &other)
        : QSharedData(other)
    {
        mId = other.mId;
        mRemoteId = other.mRemoteId;
        mRemoteRevision = other.mRemoteRevision;
        mAttributeStorage = other.mAttributeStorage;
        if (other.mParent) {
            mParent.reset(new Collection(*(other.mParent)));
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
        rootCollection.setContentMimeTypes({Collection::mimeType()});
        return rootCollection;
    }

    // Make use of the 4-bytes padding from QSharedData
    Collection::ListPreference displayPreference : 2;
    Collection::ListPreference syncPreference : 2;
    Collection::ListPreference indexPreference : 2;
    bool listPreferenceChanged : 1;
    bool enabled : 1;
    bool enabledChanged : 1;
    bool contentTypesChanged : 1;
    bool cachePolicyChanged : 1;
    bool isVirtual : 1;
    // 2 bytes padding here

    Collection::Id mId;
    QString mRemoteId;
    QString mRemoteRevision;
    mutable QScopedPointer<Collection> mParent;
    AttributeStorage mAttributeStorage;
    QString name;
    QString resource;
    CollectionStatistics statistics;
    QStringList contentTypes;
    static const Collection root;
    CachePolicy cachePolicy;
    QSet<QByteArray> keepLocalChanges;
};

