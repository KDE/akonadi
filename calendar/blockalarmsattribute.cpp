/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Tobias Koenig <tokoe@kdab.com>

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

#include "blockalarmsattribute.h"
#include <akonadi/attributefactory.h>
#include <QtCore/QByteArray>

using namespace Akonadi;

BlockAlarmsAttribute::BlockAlarmsAttribute()
{
}

BlockAlarmsAttribute::~BlockAlarmsAttribute()
{
}

QByteArray BlockAlarmsAttribute::type() const
{
  return "BlockAlarmsAttribute";
}

BlockAlarmsAttribute *BlockAlarmsAttribute::clone() const
{
  return new BlockAlarmsAttribute();
}

QByteArray BlockAlarmsAttribute::serialized() const
{
  return QByteArray();
}

void BlockAlarmsAttribute::deserialize( const QByteArray &data )
{
  Q_ASSERT( data.isEmpty() );
  Q_UNUSED( data );
}


#ifndef KDELIBS_STATIC_LIBS
namespace {

// Anonymous namespace; function is invisible outside this file.
bool dummy()
{
  Akonadi::AttributeFactory::registerAttribute<Akonadi::BlockAlarmsAttribute>();

  return true;
}

// Called when this library is loaded.
const bool registered = dummy();

} // namespace

#else

extern bool ___Akonadi____INIT()
{
  Akonadi::AttributeFactory::registerAttribute<Akonadi::BlockAlarmsAttribute>();

  return true;
}

#endif
