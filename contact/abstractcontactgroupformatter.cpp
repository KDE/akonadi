/*
    This file is part of Akonadi Contact.

    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "abstractcontactgroupformatter.h"

#include <akonadi/item.h>
#include <kabc/contactgroup.h>

using namespace Akonadi;

class AbstractContactGroupFormatter::Private
{
  public:
    KABC::ContactGroup mContactGroup;
    Akonadi::Item mItem;
    QList<QVariantMap> mAdditionalFields;
};

AbstractContactGroupFormatter::AbstractContactGroupFormatter()
  : d( new Private )
{
}

AbstractContactGroupFormatter::~AbstractContactGroupFormatter()
{
  delete d;
}

void AbstractContactGroupFormatter::setContactGroup( const KABC::ContactGroup &group )
{
  d->mContactGroup = group;
}

KABC::ContactGroup AbstractContactGroupFormatter::contactGroup() const
{
  return d->mContactGroup;
}

void AbstractContactGroupFormatter::setItem( const Akonadi::Item &item )
{
  d->mItem = item;
}

Akonadi::Item AbstractContactGroupFormatter::item() const
{
  return d->mItem;
}

void AbstractContactGroupFormatter::setAdditionalFields( const QList<QVariantMap> &fields )
{
  d->mAdditionalFields = fields;
}

QList<QVariantMap> AbstractContactGroupFormatter::additionalFields() const
{
  return d->mAdditionalFields;
}
