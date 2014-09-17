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

#include "collectionidentificationattribute.h"

#include <akonadi/private/imapparser_p.h>

#include <QByteArray>
#include <QList>

class CollectionIdentificationAttribute::Private
{
public:
    Private()
    {
    }
    QByteArray mFolderNamespace;
    QByteArray mIdentifier;
};

CollectionIdentificationAttribute::CollectionIdentificationAttribute(const QByteArray &identifier, const QByteArray &folderNamespace)
    : d(new Private)
{
    d->mIdentifier = identifier;
    d->mFolderNamespace = folderNamespace;
}

CollectionIdentificationAttribute::~CollectionIdentificationAttribute()
{
    delete d;
}

void CollectionIdentificationAttribute::setIdentifier(const QByteArray &identifier)
{
    d->mIdentifier = identifier;
}

QByteArray CollectionIdentificationAttribute::identifier() const
{
    return d->mIdentifier;
}

void CollectionIdentificationAttribute::setCollectionNamespace(const QByteArray &ns)
{
    d->mFolderNamespace = ns;
}

QByteArray CollectionIdentificationAttribute::collectionNamespace() const
{
    return d->mFolderNamespace;
}

QByteArray CollectionIdentificationAttribute::type() const
{
    return "collectionidentification";
}

Akonadi::Attribute *CollectionIdentificationAttribute::clone() const
{
    return new CollectionIdentificationAttribute(d->mIdentifier, d->mFolderNamespace);
}

QByteArray CollectionIdentificationAttribute::serialized() const
{
    QList<QByteArray> l;
    l << Akonadi::ImapParser::quote(d->mIdentifier);
    l << Akonadi::ImapParser::quote(d->mFolderNamespace);
    return '(' + Akonadi::ImapParser::join(l, " ") + ')';
}

void CollectionIdentificationAttribute::deserialize(const QByteArray &data)
{
    QList<QByteArray> l;
    Akonadi::ImapParser::parseParenthesizedList(data, l);
    const int size = l.size();
    Q_ASSERT(size >= 2);
    if (size < 2) {
        return;
    }
    d->mIdentifier = l[0];
    d->mFolderNamespace = l[1];
}
