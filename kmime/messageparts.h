/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>
                  2007 Till Adam <adam@kde.org>

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

#ifndef AKONADI_MESSAGEPARTS_H
#define AKONADI_MESSAGEPARTS_H

#include <QtCore/QLatin1String>

#include "akonadi-kmime_export.h"

namespace Akonadi
{
  class AKONADI_KMIME_EXPORT MessagePart  //krazy:exclude=dpointer
  {
    public:

      /**
        The part identifier for envelope parts.
       */
      static const QLatin1String Envelope;

      /**
        The part identifier for the main body part.
       */
      static const QLatin1String Body;

      /**
        The part identifier for the header part.
      */
      static const QLatin1String Header;
  };
}

#endif
