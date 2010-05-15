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

#include "abstractcontactformatter.h"

#include <akonadi/item.h>
#include <kabc/addressee.h>

using namespace Akonadi;

class AbstractContactFormatter::Private
{
  public:
    KABC::Addressee mContact;
    Akonadi::Item mItem;
    QList<QVariantMap> mCustomFieldDescriptions;
};

AbstractContactFormatter::AbstractContactFormatter()
  : d( new Private )
{
}

AbstractContactFormatter::~AbstractContactFormatter()
{
  delete d;
}

void AbstractContactFormatter::setContact( const KABC::Addressee &contact )
{
  d->mContact = contact;
}

KABC::Addressee AbstractContactFormatter::contact() const
{
  return d->mContact;
}

void AbstractContactFormatter::setItem( const Akonadi::Item &item )
{
  d->mItem = item;
}

Akonadi::Item AbstractContactFormatter::item() const
{
  return d->mItem;
}

void AbstractContactFormatter::setCustomFieldDescriptions( const QList<QVariantMap> &descriptions )
{
  d->mCustomFieldDescriptions = descriptions;
}

QList<QVariantMap> AbstractContactFormatter::customFieldDescriptions() const
{
  return d->mCustomFieldDescriptions;
}
