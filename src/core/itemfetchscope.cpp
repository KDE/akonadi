/*
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "itemfetchscope.h"

#include "itemfetchscope_p.h"

#include <QtCore/QStringList>

using namespace Akonadi;

ItemFetchScope::ItemFetchScope()
{
    d = new ItemFetchScopePrivate();
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

QSet< QByteArray > ItemFetchScope::payloadParts() const
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

QSet< QByteArray > ItemFetchScope::attributes() const
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
    return d->mPayloadParts.isEmpty() && d->mAttributes.isEmpty() && !d->mFullPayload && !d->mAllAttributes && !d->mFetchTags && !d->mFetchVRefs;
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
