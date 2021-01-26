/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tagfetchscope.h"
#include <QSet>

using namespace Akonadi;

struct Q_DECL_HIDDEN Akonadi::TagFetchScope::Private {
    Private()
    {
    }

    QSet<QByteArray> mAttributes;
    bool mFetchIdOnly = false;
    bool mFetchAllAttrs = true;
    bool mFetchRemotId = false;
};

TagFetchScope::TagFetchScope()
    : d(new Private)
{
}

TagFetchScope::~TagFetchScope()
{
}

TagFetchScope::TagFetchScope(const TagFetchScope &other)
    : d(new Private)
{
    operator=(other);
}

TagFetchScope &TagFetchScope::operator=(const TagFetchScope &other)
{
    d->mAttributes = other.d->mAttributes;
    d->mFetchIdOnly = other.d->mFetchIdOnly;
    d->mFetchRemotId = other.d->mFetchRemotId;
    d->mFetchAllAttrs = other.d->mFetchAllAttrs;
    return *this;
}

QSet<QByteArray> TagFetchScope::attributes() const
{
    return d->mAttributes;
}

void TagFetchScope::fetchAttribute(const QByteArray &type, bool fetch)
{
    if (fetch) {
        d->mAttributes.insert(type);
    } else {
        d->mAttributes.remove(type);
    }
}

void TagFetchScope::setFetchIdOnly(bool idOnly)
{
    d->mFetchIdOnly = idOnly;
    d->mAttributes.clear();
}

bool TagFetchScope::fetchIdOnly() const
{
    return d->mFetchIdOnly;
}

void TagFetchScope::setFetchRemoteId(bool fetchRemoteId)
{
    d->mFetchRemotId = fetchRemoteId;
}

bool TagFetchScope::fetchRemoteId() const
{
    return d->mFetchRemotId;
}

void TagFetchScope::setFetchAllAttributes(bool fetchAllAttrs)
{
    d->mFetchAllAttrs = fetchAllAttrs;
}

bool TagFetchScope::fetchAllAttributes() const
{
    return d->mFetchAllAttrs;
}
