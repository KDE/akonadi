/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include <QtCore/QDebug>

using namespace Akonadi;

class ImapParser::Private {
  public:
    QByteArray tagBuffer;
    QByteArray dataBuffer;
    int parenthesesCount;
    int literalSize;
    bool continuation;

    // returns true if readBuffer contains a literal start and sets
    // parser state accrodingly
    bool checkLiteralStart( const QByteArray &readBuffer, int pos = 0 )
    {
      if ( readBuffer.trimmed().endsWith( '}' ) ) {
        const int begin = readBuffer.lastIndexOf( '{' );
        const int end = readBuffer.lastIndexOf( '}' );

        // new literal in previous literal data block
        if ( begin < pos )
          return false;

        // TODO error handling
        literalSize = readBuffer.mid( begin + 1, end - begin - 1 ).toInt();

        // emtpy literal
        if ( literalSize == 0 )
          return false;

        continuation = true;
        return true;
      }
      return false;
    }
};

int ImapParser::parseParenthesizedList( const QByteArray & data, QList<QByteArray> &result, int start )
{
  result.clear();
  if ( start >= data.length() )
    return data.length();

  int begin = data.indexOf( '(', start );
  if ( begin < 0 )
    return start;

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
        return i + 1;
      if ( count == 1 )
        result.append( data.mid( sublistbegin, i - sublistbegin + 1 ) );
      --count;
      continue;
    }
    if ( data[i] == ' ' )
      continue;
    if ( count == 0 ) {
      QByteArray ba;
      i = parseString( data, ba, i ) - 1; // compensate the increment
      result.append( ba );
    }
  }
  return data.length();
}

int ImapParser::parseString( const QByteArray & data, QByteArray & result, int start )
{
  int begin = stripLeadingSpaces( data, start );
  result.clear();
  if ( begin >= data.length() )
    return data.length();

  // literal string
  // TODO: error handling
  if ( data[begin] == '{' ) {
    int end = data.indexOf( '}', begin );
    Q_ASSERT( end > begin );
    int size = data.mid( begin + 1, end - begin - 1 ).toInt();

    // strip CRLF
    begin = end + 1;
    if ( begin < data.length() && data[begin] == '\r' )
      ++begin;
    if ( begin < data.length() && data[begin] == '\n' )
      ++begin;

    end = begin + size;
    result = data.mid( begin, end - begin );
    return end;
  }

  // quoted string
  return parseQuotedString( data, result, begin );
}

int ImapParser::parseQuotedString( const QByteArray & data, QByteArray &result, int start )
{
  int begin = stripLeadingSpaces( data, start );
  int end = begin;
  result.clear();
  if ( begin >= data.length() )
    return data.length();

  // quoted string
  if ( data[begin] == '"' ) {
    ++begin;
    for ( int i = begin; i < data.length(); ++i ) {
      if ( data[i] == '\\' ) {
        ++i;
        continue;
      }
      if ( data[i] == '"' ) {
        result = data.mid( begin, i - begin );
        end = i + 1; // skip the '"'
        break;
      }
    }
  }

  // unquoted string
  else {
    bool reachedInputEnd = true;
    for ( int i = begin; i < data.length(); ++i ) {
      if ( data[i] == ' ' || data[i] == '(' || data[i] == ')' || data[i] == '\n' || data[i] == '\r' ) {
        end = i;
        reachedInputEnd = false;
        break;
      }
    }
    if ( reachedInputEnd )
      end = data.length();
    result = data.mid( begin, end - begin );

    // transform unquoted NIL
    if ( result == "NIL" )
      result.clear();
  }

  // strip quotes
  // FIXME: this can be done more efficient
  while ( result.contains( "\\\"" ) )
    result.replace( "\\\"", "\"" );
  while ( result.contains( "\\\\" ) )
    result.replace( "\\\\", "\\" );

  return end;
}

int ImapParser::stripLeadingSpaces( const QByteArray & data, int start )
{
  for ( int i = start; i < data.length(); ++i ) {
    if ( data[i] != ' ' )
      return i;
  }
  return data.length();
}

int ImapParser::parenthesesBalance( const QByteArray & data, int start )
{
  int count = 0;
  bool insideQuote = false;
  for ( int i = start; i < data.length(); ++i ) {
    if ( data[i] == '"' ) {
      insideQuote = !insideQuote;
      continue;
    }
    if ( data[i] == '\\' && insideQuote ) {
      ++i;
      continue;
    }
    if ( data[i] == '(' && !insideQuote ) {
      ++count;
      continue;
    }
    if ( data[i] == ')' && !insideQuote ) {
      --count;
      continue;
    }
  }
  return count;
}

QByteArray ImapParser::join(const QList< QByteArray > & list, const QByteArray & separator)
{
  if ( list.isEmpty() )
    return QByteArray();

  QByteArray result = list.first();
  QList<QByteArray>::ConstIterator it = list.constBegin();
  ++it;
  for ( ; it != list.constEnd(); ++it )
    result += separator + (*it);

  return result;
}

int ImapParser::parseString(const QByteArray & data, QString & result, int start)
{
  QByteArray tmp;
  int end = parseString( data, tmp, start );
  result = QString::fromUtf8( tmp );
  return end;
}

int ImapParser::parseNumber(const QByteArray & data, int & result, bool * ok, int start)
{
  if ( ok )
    *ok = false;
  int pos = stripLeadingSpaces( data, start );
  if ( pos >= data.length() )
    return data.length();
  QByteArray tmp;
  for (; pos < data.length(); ++pos ) {
    if ( !QChar( QLatin1Char( data.at( pos ) ) ).isDigit() )
      break;
    tmp += data.at( pos );
  }
  result = tmp.toInt( ok );
  return pos;
}

QByteArray ImapParser::quote(const QByteArray & data)
{
  QByteArray result( "\"" );
  result.reserve( data.length() + 2 );
  for ( int i = 0; i < data.length(); ++i ) {
    if ( data.at( i ) == '"' || data.at( i ) == '\\' )
      result += '\\';
    result += data.at( i );
  }
  result += '"';
  return result;
}

ImapParser::ImapParser() :
    d ( new Private )
{
  reset();
}

ImapParser::~ ImapParser()
{
  delete d;
}

bool ImapParser::parseNextLine(const QByteArray &readBuffer)
{
  d->continuation = false;

  // first line, get the tag
  if ( d->tagBuffer.isEmpty() ) {
    const int startOfData = ImapParser::parseString( readBuffer, d->tagBuffer );
    if ( startOfData < readBuffer.length() && startOfData >= 0 )
      d->dataBuffer = readBuffer.mid( startOfData + 1 );

  } else {
    d->dataBuffer += readBuffer;
  }

  // literal read in progress
  if ( d->literalSize > 0 ) {
    d->literalSize -= readBuffer.size();

    // still not everything read
    if ( d->literalSize > 0 )
      return false;

    // check the remaining (non-literal) part for parentheses
    if ( d->literalSize < 0 ) {
      // the following looks strange but works since literalSize can be negative here
      d->parenthesesCount = ImapParser::parenthesesBalance( readBuffer, readBuffer.length() + d->literalSize );

      // check if another literal read was started
      if ( d->checkLiteralStart( readBuffer, readBuffer.length() + d->literalSize ) )
        return false;
    }

    // literal string finished but still open parentheses
    if ( d->parenthesesCount > 0 )
        return false;

  } else {

    // open parentheses
    d->parenthesesCount += ImapParser::parenthesesBalance( readBuffer );

    // start new literal read
    if ( d->checkLiteralStart( readBuffer ) )
      return false;

    // still open parentheses
    if ( d->parenthesesCount > 0 )
      return false;

    // just a normal response, fall through
  }

  return true;
}

QByteArray ImapParser::tag() const
{
  return d->tagBuffer;
}

QByteArray ImapParser::data() const
{
  return d->dataBuffer;
}

void ImapParser::reset()
{
  d->dataBuffer.clear();
  d->tagBuffer.clear();
  d->parenthesesCount = 0;
  d->literalSize = 0;
  d->continuation = false;
}

bool ImapParser::continuationStarted() const
{
  return d->continuation;
}
