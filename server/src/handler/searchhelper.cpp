/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "searchhelper.h"

using namespace Akonadi;

QList<QByteArray> SearchHelper::splitLine( const QByteArray &line )
{
  QList<QByteArray> retval;

  int i, start = 0;
  bool escaped = false;
  for ( i = 0; i < line.count(); ++i ) {
    if ( line[ i ] == ' ' ) {
      if ( !escaped ) {
        retval.append( line.mid( start, i - start ) );
        start = i + 1;
      }
    } else if ( line[ i ] == '"' ) {
      if ( escaped ) {
        escaped = false;
      } else {
        escaped = true;
      }
    }
  }

  retval.append( line.mid( start, i - start ) );

  return retval;
}

QByteArray SearchHelper::extractMimetype( const QList<QByteArray> &junks, int start )
{
  QByteArray mimeType;

  if ( junks.count() <= start )
    return QByteArray();

  if ( junks[ start ].toUpper() == "CHARSET" ) {
    if ( junks.count() <= ( start + 2 ) )
      return QByteArray();
    if ( junks[ start + 2 ].toUpper() == "MIMETYPE" ) {
      if ( junks.count() <= ( start + 3 ) )
        return QByteArray();
      else
        mimeType = junks[ start + 3 ].toLower();
    }
  } else {
    if ( junks[ start ].toUpper() == "MIMETYPE" ) {
      if ( junks.count() <= ( start + 1 ) )
        return QByteArray();
      else
        mimeType = junks[ start + 1 ].toLower();
    }
  }

  if ( mimeType.isEmpty() )
    mimeType = "message/rfc822";

  return mimeType;
}
