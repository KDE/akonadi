/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "collectiondisplayattribute.h"

#include "imapparser_p.h"

#include <KIcon>

using namespace Akonadi;

class CollectionDisplayAttribute::Private
{
  public:
    QString name;
    QString icon;
};

CollectionDisplayAttribute::CollectionDisplayAttribute() :
    d( new Private )
{
}

CollectionDisplayAttribute::~ CollectionDisplayAttribute()
{
  delete d;
}

QString CollectionDisplayAttribute::displayName() const
{
  return d->name;
}

void CollectionDisplayAttribute::setDisplayName(const QString & name)
{
  d->name = name;
}

KIcon CollectionDisplayAttribute::icon() const
{
  return KIcon( d->icon );
}

QString CollectionDisplayAttribute::iconName() const
{
  return d->icon;
}

void CollectionDisplayAttribute::setIconName(const QString & icon)
{
  d->icon = icon;
}

QByteArray Akonadi::CollectionDisplayAttribute::type() const
{
  return "COLDISPLAY";
}

CollectionDisplayAttribute * CollectionDisplayAttribute::clone() const
{
  CollectionDisplayAttribute *attr = new CollectionDisplayAttribute();
  attr->d->name = d->name;
  attr->d->icon = d->icon;
  return attr;
}

QByteArray CollectionDisplayAttribute::serialized() const
{
  QList<QByteArray> l;
  l << ImapParser::quote( d->name.toUtf8() );
  l << ImapParser::quote( d->icon.toUtf8() );
  return "(" + ImapParser::join( l, " " ) + ")";
}

void CollectionDisplayAttribute::deserialize(const QByteArray &data)
{
  QList<QByteArray> l;
  ImapParser::parseParenthesizedList( data, l );
  Q_ASSERT( l.count() >= 2 );
  d->name = QString::fromUtf8( l[0] );
  d->icon = QString::fromUtf8( l[1] );
}
