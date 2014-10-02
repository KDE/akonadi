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

#include "imapparser_p.h"

#include <QByteArray>
#include <QList>

#include <KDebug>

class CollectionIdentificationAttribute::Private
{
public:
    Private() {}
    QByteArray mFolderNamespace;
    QByteArray mIdentifier;
    QByteArray mName;
    QByteArray mOrganizationUnit;
    QByteArray mMail;
};

CollectionIdentificationAttribute::CollectionIdentificationAttribute(const QByteArray &identifier, const QByteArray &folderNamespace,
                                                                     const QByteArray &name, const QByteArray &organizationUnit, const QByteArray &mail)
    :d(new Private)
{
    d->mIdentifier = identifier;
    d->mFolderNamespace = folderNamespace;
    d->mName = name;
    d->mOrganizationUnit = organizationUnit;
    d->mMail = mail;
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

void CollectionIdentificationAttribute::setMail(const QByteArray &mail)
{
    d->mMail = mail;
}

QByteArray CollectionIdentificationAttribute::mail() const
{
    return d->mMail;
}

void CollectionIdentificationAttribute::setOu(const QByteArray &ou)
{
    d->mOrganizationUnit = ou;
}

QByteArray CollectionIdentificationAttribute::ou() const
{
    return d->mOrganizationUnit;
}

void CollectionIdentificationAttribute::setName(const QByteArray &name)
{
    d->mName = name;
}

QByteArray CollectionIdentificationAttribute::name() const
{
    return d->mName;
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

Akonadi::Attribute* CollectionIdentificationAttribute::clone() const
{
    return new CollectionIdentificationAttribute(d->mIdentifier, d->mFolderNamespace, d->mName, d->mOrganizationUnit, d->mMail);
}

QByteArray CollectionIdentificationAttribute::serialized() const
{
    QList<QByteArray> l;
    l << Akonadi::ImapParser::quote(d->mIdentifier);
    l << Akonadi::ImapParser::quote(d->mFolderNamespace);
    l << Akonadi::ImapParser::quote(d->mName);
    l << Akonadi::ImapParser::quote(d->mOrganizationUnit);
    l << Akonadi::ImapParser::quote(d->mMail);
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

    if (size == 5) {
        d->mName = l[2];
        d->mOrganizationUnit = l[3];
        d->mMail = l[4];
    }
}

