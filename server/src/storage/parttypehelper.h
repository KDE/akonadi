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

#ifndef PARTTYPEHELPER_H
#define PARTTYPEHELPER_H

#include "exception.h"

namespace Akonadi {

class PartType;

AKONADI_EXCEPTION_MAKE_INSTANCE(PartTypeException);

/**
 * Methods for dealing with the PartType table.
 */
namespace PartTypeHelper
{
  /**
   * Retrieve (or create) PartType for the given fully qualified name.
   * @param fqName Fully qualified name (NS:NAME).
   * @throws PartTypeException
   */
  PartType fromFqName( const QString &fqName );

  /**
   * Convenience overload of the above.
   */
  PartType fromFqName( const QByteArray &fqName );

  /**
   * Retrieve (or create) PartType for the given namespace and type name.
   * @param ns Namespace
   * @param typeName Part type name.
   * @throws PartTypeException
   */
  PartType fromName( const QString &ns, const QString &typeName );

  /**
   * Convenience overload of the above.
   */
  PartType fromName( const char *ns, const char *typeName );
}

}

#endif
