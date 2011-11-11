/*
    Copyright (c) 2011 Volker Krause <vkrause@kde.org>

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

#include "parttypehelper.h"

#include "selectquerybuilder.h"
#include "entities.h"

#include <QtCore/QStringList>

using namespace Akonadi;

PartType PartTypeHelper::fromFqName(const QString& fqName)
{
  const QStringList name = fqName.split( QLatin1Char(':') );
  if ( name.size() != 2 )
    throw PartTypeException( "Invalid part type name." );
  return fromName( name.first(), name.last() );
}

PartType PartTypeHelper::fromFqName(const QByteArray& fqName)
{
  return fromFqName( QLatin1String(fqName) );
}

PartType PartTypeHelper::fromName(const QString& ns, const QString& typeName)
{
  SelectQueryBuilder<PartType> qb;
  qb.addValueCondition( PartType::nsColumn(), Query::Equals, ns );
  qb.addValueCondition( PartType::nameColumn(), Query::Equals, typeName );
  if ( !qb.exec() )
    throw PartTypeException( "Unable to query part type table." );
  const PartType::List result = qb.result();
  if ( result.size() == 1 )
    return result.first();
  if ( result.size() > 1 )
    throw PartTypeException( "Part type uniqueness constraint violation." );

  // doesn't exist yet, so let's create a new one
  PartType type;
  type.setName( typeName );
  type.setNs( ns );
  if ( !type.insert() )
    throw PartTypeException( "Creating a new part type failed." );
  return type;
}

PartType PartTypeHelper::fromName(const char* ns, const char* typeName)
{
  return fromName( QLatin1String(ns), QLatin1String(typeName) );
}
