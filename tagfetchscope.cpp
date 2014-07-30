/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include "tagfetchscope.h"
#include <QSet>

using namespace Akonadi;

struct Akonadi::TagFetchScope::Private
{
    Private()
    : mFetchIdOnly(false)
    {
    }

    QSet<QByteArray> mAttributes;
    bool mFetchIdOnly;
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

