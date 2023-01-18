/*
    SPDX-FileCopyrightText: 2008 Kevin Krammer <kevin.krammer@gmx.at>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemfetchscope.h"

#include "itemfetchscope_p.h"

using namespace Akonadi;

ItemFetchScope::ItemFetchScope()
    : d(new ItemFetchScopePrivate())
{
}

ItemFetchScope::ItemFetchScope(const ItemFetchScope &other)
    : d(other.d)
{
}

ItemFetchScope::~ItemFetchScope()
{
}

ItemFetchScope &ItemFetchScope::operator=(const ItemFetchScope &other)
{
    if (&other != this) {
        d = other.d;
    }

    return *this;
}

QSet<QByteArray> ItemFetchScope::payloadParts() const
{
    return d->mPayloadParts;
}

void ItemFetchScope::fetchPayloadPart(const QByteArray &part, bool fetch)
{
    if (fetch) {
        d->mPayloadParts.insert(part);
    } else {
        d->mPayloadParts.remove(part);
    }
}

bool ItemFetchScope::fullPayload() const
{
    return d->mFullPayload;
}

void ItemFetchScope::fetchFullPayload(bool fetch)
{
    d->mFullPayload = fetch;
}

QSet<QByteArray> ItemFetchScope::attributes() const
{
    return d->mAttributes;
}

void ItemFetchScope::fetchAttribute(const QByteArray &type, bool fetch)
{
    if (fetch) {
        d->mAttributes.insert(type);
    } else {
        d->mAttributes.remove(type);
    }
}

bool ItemFetchScope::allAttributes() const
{
    return d->mAllAttributes;
}

void ItemFetchScope::fetchAllAttributes(bool fetch)
{
    d->mAllAttributes = fetch;
}

bool ItemFetchScope::isEmpty() const
{
    return d->mPayloadParts.isEmpty() && d->mAttributes.isEmpty() && !d->mFullPayload && !d->mAllAttributes && !d->mCacheOnly
        && !d->mCheckCachedPayloadPartsOnly && d->mFetchMtime // true by default -> false = non-empty
        && !d->mIgnoreRetrievalErrors && d->mFetchRid // true by default
        && !d->mFetchGid && !d->mFetchTags && !d->mFetchVRefs && !d->mFetchRelations && d->mAncestorDepth == AncestorRetrieval::None;
}

bool ItemFetchScope::cacheOnly() const
{
    return d->mCacheOnly;
}

void ItemFetchScope::setCacheOnly(bool cacheOnly)
{
    d->mCacheOnly = cacheOnly;
}

void ItemFetchScope::setCheckForCachedPayloadPartsOnly(bool check)
{
    if (check) {
        setCacheOnly(true);
    }
    d->mCheckCachedPayloadPartsOnly = check;
}

bool ItemFetchScope::checkForCachedPayloadPartsOnly() const
{
    return d->mCheckCachedPayloadPartsOnly;
}

ItemFetchScope::AncestorRetrieval ItemFetchScope::ancestorRetrieval() const
{
    return d->mAncestorDepth;
}

void ItemFetchScope::setAncestorRetrieval(AncestorRetrieval depth)
{
    d->mAncestorDepth = depth;
}

void ItemFetchScope::setFetchModificationTime(bool retrieveMtime)
{
    d->mFetchMtime = retrieveMtime;
}

bool ItemFetchScope::fetchModificationTime() const
{
    return d->mFetchMtime;
}

void ItemFetchScope::setFetchGid(bool retrieveGid)
{
    d->mFetchGid = retrieveGid;
}

bool ItemFetchScope::fetchGid() const
{
    return d->mFetchGid;
}

void ItemFetchScope::setIgnoreRetrievalErrors(bool ignore)
{
    d->mIgnoreRetrievalErrors = ignore;
}

bool ItemFetchScope::ignoreRetrievalErrors() const
{
    return d->mIgnoreRetrievalErrors;
}

void ItemFetchScope::setFetchChangedSince(const QDateTime &changedSince)
{
    d->mChangedSince = changedSince;
}

QDateTime ItemFetchScope::fetchChangedSince() const
{
    return d->mChangedSince;
}

void ItemFetchScope::setFetchRemoteIdentification(bool retrieveRid)
{
    d->mFetchRid = retrieveRid;
}

bool ItemFetchScope::fetchRemoteIdentification() const
{
    return d->mFetchRid;
}

void ItemFetchScope::setFetchTags(bool fetchTags)
{
    d->mFetchTags = fetchTags;
}

bool ItemFetchScope::fetchTags() const
{
    return d->mFetchTags;
}

void ItemFetchScope::setTagFetchScope(const TagFetchScope &tagFetchScope)
{
    d->mTagFetchScope = tagFetchScope;
}

TagFetchScope &ItemFetchScope::tagFetchScope()
{
    return d->mTagFetchScope;
}

TagFetchScope ItemFetchScope::tagFetchScope() const
{
    return d->mTagFetchScope;
}

void ItemFetchScope::setFetchVirtualReferences(bool fetchVRefs)
{
    d->mFetchVRefs = fetchVRefs;
}

bool ItemFetchScope::fetchVirtualReferences() const
{
    return d->mFetchVRefs;
}

void ItemFetchScope::setFetchRelations(bool fetchRelations)
{
    d->mFetchRelations = fetchRelations;
}

bool ItemFetchScope::fetchRelations() const
{
    return d->mFetchRelations;
}
