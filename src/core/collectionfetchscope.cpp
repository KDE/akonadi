/*
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "collectionfetchscope.h"

#include <QString>
#include <QStringList>
#include <QScopedPointer>

namespace Akonadi
{

class CollectionFetchScopePrivate : public QSharedData
{
public:
    CollectionFetchScopePrivate()
        : ancestorDepth(CollectionFetchScope::None)
        , listFilter(CollectionFetchScope::Enabled)
        , statistics(false)
        , fetchIdOnly(true)
        , mIgnoreRetrievalErrors(false)
    {
    }

    CollectionFetchScopePrivate(const CollectionFetchScopePrivate &other)
        : QSharedData(other)
    {
        resource = other.resource;
        contentMimeTypes = other.contentMimeTypes;
        ancestorDepth = other.ancestorDepth;
        statistics = other.statistics;
        listFilter = other.listFilter;
        attributes = other.attributes;
        if (!ancestorFetchScope && other.ancestorFetchScope) {
            ancestorFetchScope.reset(new CollectionFetchScope());
            *ancestorFetchScope = *other.ancestorFetchScope;
        } else if (ancestorFetchScope && !other.ancestorFetchScope) {
            ancestorFetchScope.reset(nullptr);
        }
        fetchIdOnly = other.fetchIdOnly;
        mIgnoreRetrievalErrors = other.mIgnoreRetrievalErrors;
    }

public:
    QString resource;
    QStringList contentMimeTypes;
    CollectionFetchScope::AncestorRetrieval ancestorDepth;
    CollectionFetchScope::ListFilter listFilter;
    QSet<QByteArray> attributes;
    QScopedPointer<CollectionFetchScope> ancestorFetchScope;
    bool statistics;
    bool fetchIdOnly;
    bool mIgnoreRetrievalErrors;
};

CollectionFetchScope::CollectionFetchScope()
    : d(new CollectionFetchScopePrivate())
{
}

CollectionFetchScope::CollectionFetchScope(const CollectionFetchScope &other)
    : d(other.d)
{
}

CollectionFetchScope::~CollectionFetchScope()
{
}

CollectionFetchScope &CollectionFetchScope::operator=(const CollectionFetchScope &other)
{
    if (&other != this) {
        d = other.d;
    }

    return *this;
}

bool CollectionFetchScope::isEmpty() const
{
    return d->resource.isEmpty() && d->contentMimeTypes.isEmpty() && !d->statistics && d->ancestorDepth == None && d->listFilter == Enabled;
}

bool CollectionFetchScope::includeStatistics() const
{
    return d->statistics;
}

void CollectionFetchScope::setIncludeStatistics(bool include)
{
    d->statistics = include;
}

QString CollectionFetchScope::resource() const
{
    return d->resource;
}

void CollectionFetchScope::setResource(const QString &resource)
{
    d->resource = resource;
}

QStringList CollectionFetchScope::contentMimeTypes() const
{
    return d->contentMimeTypes;
}

void CollectionFetchScope::setContentMimeTypes(const QStringList &mimeTypes)
{
    d->contentMimeTypes = mimeTypes;
}

CollectionFetchScope::AncestorRetrieval CollectionFetchScope::ancestorRetrieval() const
{
    return d->ancestorDepth;
}

void CollectionFetchScope::setAncestorRetrieval(AncestorRetrieval ancestorDepth)
{
    d->ancestorDepth = ancestorDepth;
}

CollectionFetchScope::ListFilter CollectionFetchScope::listFilter() const
{
    return d->listFilter;
}

void CollectionFetchScope::setListFilter(CollectionFetchScope::ListFilter listFilter)
{
    d->listFilter = listFilter;
}

QSet<QByteArray> CollectionFetchScope::attributes() const
{
    return d->attributes;
}

void CollectionFetchScope::fetchAttribute(const QByteArray &type, bool fetch)
{
    d->fetchIdOnly = false;
    if (fetch) {
        d->attributes.insert(type);
    } else {
        d->attributes.remove(type);
    }
}

void CollectionFetchScope::setFetchIdOnly(bool fetchIdOnly)
{
    d->fetchIdOnly = fetchIdOnly;
}

bool CollectionFetchScope::fetchIdOnly() const
{
    return d->fetchIdOnly;
}

void CollectionFetchScope::setIgnoreRetrievalErrors(bool enable)
{
    d->mIgnoreRetrievalErrors = enable;
}

bool CollectionFetchScope::ignoreRetrievalErrors() const
{
    return d->mIgnoreRetrievalErrors;
}

void CollectionFetchScope::setAncestorFetchScope(const CollectionFetchScope &scope)
{
    *d->ancestorFetchScope = scope;
}

CollectionFetchScope CollectionFetchScope::ancestorFetchScope() const
{
    if (!d->ancestorFetchScope) {
        return CollectionFetchScope();
    }
    return *d->ancestorFetchScope;
}

CollectionFetchScope &CollectionFetchScope::ancestorFetchScope()
{
    if (!d->ancestorFetchScope) {
        d->ancestorFetchScope.reset(new CollectionFetchScope());
    }
    return *d->ancestorFetchScope;
}

} // namespace Akonadi
