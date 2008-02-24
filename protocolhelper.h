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

#ifndef AKONADI_PROTOCOLHELPER_H
#define AKONADI_PROTOCOLHELPER_H

#include <libakonadi/cachepolicy.h>
#include <libakonadi/collection.h>

namespace Akonadi {

/**
  @internal
  Helper methods for converting between libakonadi objects and their protocol
  representation.

  @todo Add unit tests for this.
  @todo Use exceptions for a useful error handling
*/
class ProtocolHelper
{
  public:
    /**
      Parse a cache policy definition.
      @param data The input data.
      @param policy The parsed cache policy.
      @param start Start of the data, ie. postion after the label
      @returns Position in data after the cache policy description.
    */
    static int parseCachePolicy( const QByteArray &data, CachePolicy &policy, int start = 0 );

    /**
      Convert a cache policy object into its protocol representation.
    */
    static QByteArray cachePolicyToByteArray( const CachePolicy &policy );

    /**
      Parse a collection description.
      @param data The input data.
      @param collection The parsed collection
      @param start Start of the data
      @returns Position in data after the collection description.
    */
    static int parseCollection( const QByteArray &data, Collection &collection, int start = 0 );

};

}

#endif
