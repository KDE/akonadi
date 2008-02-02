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

#include "protocolhelper.h"

#include "imapparser.h"

using namespace Akonadi;

int ProtocolHelper::parseCachePolicy(const QByteArray & data, CachePolicy & policy, int start)
{
  QList<QByteArray> params;
  int end = Akonadi::ImapParser::parseParenthesizedList( data, params, start );
  for ( int i = 0; i < params.count() - 1; i += 2 ) {
    const QByteArray key = params[i];
    const QByteArray value = params[i + 1];

    if ( key == "INHERIT" )
      policy.setInheritFromParent( value == "true" );
    else if ( key == "INTERVAL" )
      policy.setIntervalCheckTime( value.toInt() );
    else if ( key == "CACHETIMEOUT" )
      policy.setCacheTimeout( value.toInt() );
    else if ( key == "SYNCONDEMAND" )
      policy.enableSyncOnDemand( value == "true" );
    else if ( key == "LOCALPARTS" ) {
      QList<QByteArray> tmp;
      QStringList parts;
      Akonadi::ImapParser::parseParenthesizedList( value, tmp );
      foreach ( const QByteArray ba, tmp )
        parts << QString::fromLatin1( ba );
      policy.setLocalParts( parts );
    }
  }
  return end;
}

QByteArray ProtocolHelper::cachePolicyToByteArray(const CachePolicy & policy)
{
  QByteArray rv = "CACHEPOLICY (";
  if ( policy.inheritFromParent() ) {
    rv += "INHERIT true";
  } else {
    rv += "INHERIT false";
    rv += " INTERVAL " + QByteArray::number( policy.intervalCheckTime() );
    rv += " CACHETIMEOUT " + QByteArray::number( policy.cacheTimeout() );
    rv += " SYNCONDEMAND " + ( policy.syncOnDemand() ? QByteArray("true") : QByteArray("false") );
    rv += " LOCALPARTS (" + policy.localParts().join( QLatin1String(" ") ).toLatin1() + ")";
  }
  rv += ")";
  return rv;
}
