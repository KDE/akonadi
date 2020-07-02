/*
    SPDX-FileCopyrightText: 2008 Kevin Krammer <kevin.krammer@gmx.at>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef ITEMFETCHSCOPE_P_H
#define ITEMFETCHSCOPE_P_H

#include <QSet>
#include <QDateTime>
#include "itemfetchscope.h"
#include "tagfetchscope.h"

namespace Akonadi
{

/**
 * @internal
 */
class ItemFetchScopePrivate : public QSharedData
{
public:
    ItemFetchScopePrivate()
        : mAncestorDepth(ItemFetchScope::None)
        , mFullPayload(false)
        , mAllAttributes(false)
        , mCacheOnly(false)
        , mCheckCachedPayloadPartsOnly(false)
        , mFetchMtime(true)
        , mIgnoreRetrievalErrors(false)
        , mFetchRid(true)
        , mFetchGid(false)
        , mFetchTags(false)
        , mFetchVRefs(false)
        , mFetchRelations(false)
    {
        mTagFetchScope.setFetchIdOnly(true);
    }

    ItemFetchScopePrivate(const ItemFetchScopePrivate &other)
        : QSharedData(other)
    {
        mPayloadParts = other.mPayloadParts;
        mAttributes = other.mAttributes;
        mAncestorDepth = other.mAncestorDepth;
        mFullPayload = other.mFullPayload;
        mAllAttributes = other.mAllAttributes;
        mCacheOnly = other.mCacheOnly;
        mCheckCachedPayloadPartsOnly = other.mCheckCachedPayloadPartsOnly;
        mFetchMtime = other.mFetchMtime;
        mIgnoreRetrievalErrors = other.mIgnoreRetrievalErrors;
        mChangedSince = other.mChangedSince;
        mFetchRid = other.mFetchRid;
        mFetchGid = other.mFetchGid;
        mFetchTags = other.mFetchTags;
        mTagFetchScope = other.mTagFetchScope;
        mFetchVRefs = other.mFetchVRefs;
        mFetchRelations = other.mFetchRelations;
    }

public:
    QSet<QByteArray> mPayloadParts;
    QSet<QByteArray> mAttributes;
    ItemFetchScope::AncestorRetrieval mAncestorDepth;
    bool mFullPayload;
    bool mAllAttributes;
    bool mCacheOnly;
    bool mCheckCachedPayloadPartsOnly;
    bool mFetchMtime;
    bool mIgnoreRetrievalErrors;
    QDateTime mChangedSince;
    bool mFetchRid;
    bool mFetchGid;
    bool mFetchTags;
    TagFetchScope mTagFetchScope;
    bool mFetchVRefs;
    bool mFetchRelations;
};

}

#endif
