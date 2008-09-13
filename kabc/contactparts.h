/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_CONTACTPARTS_H
#define AKONADI_CONTACTPARTS_H

#include "akonadi-kabc_export.h"

namespace Akonadi
{
  /**
   * @short Contains predefined part identifiers.
   *
   * This namespace contains identifiers of item parts that are used for
   * handling contact items.
   */
  namespace ContactPart
  {
      /**
       * The part identifier for a small contact version,
       * that contains only name and email addresses.
       * @since 4.2
       */
      AKONADI_KABC_EXPORT extern const char* Lookup;

      /**
       * The part identifier for all the contact data except
       * images and sounds.
       *
       * @note Use Akonadi::Item::FullPayload to retrieve the
       *       full contact including images and sounds.
       * @since 4.2
       */
      AKONADI_KABC_EXPORT extern const char* Standard;
  }
}

#endif
