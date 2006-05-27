/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include "imapparser.h"

#include <QDebug>

using namespace PIM;

QList< QByteArray > PIM::ImapParser::parseParentheziedList( const QByteArray & data, int start )
{
  QList<QByteArray> rv;
  int begin = data.indexOf( '(', start );
  int count = 0;
  int sublistbegin = start;
  for ( int i = begin + 1; i < data.length(); ++i ) {
    if ( data[i] == '(' ) {
      ++count;
      if ( count == 1 )
        sublistbegin = i;
      continue;
    }
    if ( data[i] == ')' ) {
      if ( count <= 0 )
        break;
      if ( count == 1 )
        rv.append( data.mid( sublistbegin, i - sublistbegin + 1 ) );
      --count;
      continue;
    }
    if ( data[i] == ' ' )
      continue;
    if ( count == 0 ) {
      QByteArray ba = parseQuotedString( data, i );
      rv.append( ba );
      if ( data[i] == '"' )
        i += 2;
      i += ba.length();
    }
  }
  return rv;
}

QByteArray PIM::ImapParser::parseQuotedString( const QByteArray & data, int start )
{
  int begin = start;
  int end = start;

  // strip leading spaces
  for ( int i = start; i < data.length(); ++i ) {
    if ( data[i] == ' ' )
        ++begin;
    else
      break;
  }

  // quoted string
  if ( data[begin] == '"' ) {
    ++begin;
    for ( int i = begin; i < data.length(); ++i ) {
      if ( data[i] == '\\' ) {
        ++i;
        continue;
      }
      if ( data[i] == '"' ) {
        end = i;
        break;
      }
    }
  }

  // unquoted string
  else {
    for ( int i = begin; i < data.length(); ++i ) {
      if ( data[i] == ' ' || data[i] == '(' || data[i] == ')' ) {
        end = i;
        break;
      }
    }
  }

  return data.mid( begin, end - begin );
}

