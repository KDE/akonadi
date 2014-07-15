/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_XML_FORMAT_P_H
#define AKONADI_XML_FORMAT_P_H

#include <QString>

namespace Akonadi {

namespace Format {

namespace Tag {

  inline QString root() { return QString::fromLatin1( "knut" ); }
  inline QString collection() { return QString::fromLatin1( "collection" ); }
  inline QString item() { return QString::fromLatin1( "item" ); }
  inline QString attribute() { return QString::fromLatin1( "attribute" ); }
  inline QString flag() { return QString::fromLatin1( "flag" ); }
  inline QString tag() { return QString::fromLatin1( "tag" ); }
  inline QString payload() { return QString::fromLatin1( "payload" ); }

}

namespace Attr {

  inline QString remoteId() { return QString::fromLatin1( "rid" ); }
  inline QString attributeType() { return QString::fromLatin1( "type" ); }
  inline QString collectionName() { return QString::fromLatin1( "name" ); }
  inline QString collectionContentTypes() { return QString::fromLatin1( "content" ); }
  inline QString itemMimeType() { return QString::fromLatin1( "mimetype" ); }
  inline QString name() { return QString::fromLatin1( "name" ); }
  inline QString gid() { return QString::fromLatin1( "gid" ); }
  inline QString type() { return QString::fromLatin1( "type" ); }

}

}

}

#endif
