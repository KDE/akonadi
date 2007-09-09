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

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <ctype.h>

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
    if ( !isdigit( data.at( pos ) ) )
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

int ImapParser::parseSequenceSet(const QByteArray & data, ImapSet & result, int start)
{
  int begin = stripLeadingSpaces( data, start );
  int value = -1, lower = -1, upper = -1;
  for ( int i = begin; i < data.length(); ++i ) {
    if ( data[i] == '*' ) {
      value = 0;
    } else if ( data[i] == ':' ) {
      lower = value;
    } else if ( isdigit( data[i] ) ) {
      bool ok = false;
      i = parseNumber( data, value, &ok, i );
      Q_ASSERT( ok ); // TODO handle error
      --i;
    } else {
      upper = value;
      if ( lower < 0 )
        lower = value;
      result.add( ImapInterval( lower, upper ) );
      lower = -1;
      upper = -1;
      value = -1;
      if ( data[i] != ',' )
        return i;
    }
  }
  // take care of left-overs at input end
  upper = value;
  if ( lower < 0 )
    lower = value;
  if ( lower >= 0 && upper >= 0 )
    result.add( ImapInterval( lower, upper ) );
  return data.length();
}

int ImapParser::parseDateTime(const QByteArray & data, QDateTime & dateTime, int start)
{
  // Syntax:
  // date-time      = DQUOTE date-day-fixed "-" date-month "-" date-year
  //                  SP time SP zone DQUOTE
  // date-day-fixed = (SP DIGIT) / 2DIGIT
  //                    ; Fixed-format version of date-day
  // date-month     = "Jan" / "Feb" / "Mar" / "Apr" / "May" / "Jun" /
  //                  "Jul" / "Aug" / "Sep" / "Oct" / "Nov" / "Dec"
  // date-year      = 4DIGIT
  // time           = 2DIGIT ":" 2DIGIT ":" 2DIGIT
  //                    ; Hours minutes seconds
  // zone           = ("+" / "-") 4DIGIT
  //                    ; Signed four-digit value of hhmm representing
  //                    ; hours and minutes east of Greenwich (that is,
  //                    ; the amount that the given time differs from
  //                    ; Universal Time).  Subtracting the timezone
  //                    ; from the given time will give the UT form.
  //                    ; The Universal Time zone is "+0000".
  // Example : "28-May-2006 01:03:35 +0200"
  // Position: 0123456789012345678901234567
  //                     1         2

  int pos = stripLeadingSpaces( data, start );
  if ( data.length() <= pos )
    return pos;
  bool quoted = false;
  if ( data[pos] == '"' ) {
    quoted = true;
    ++pos;
  }
  if ( data.length() <= pos + 26 )
    return start;

  bool ok = true;
  const int day = ( data[pos] == ' ' ? data[pos + 1] - '0' // single digit day
                                     : data.mid( pos, 2 ).toInt( &ok ) );
  if ( !ok ) return start;
  pos += 3;
  const QByteArray shortMonthNames( "janfebmaraprmayjunjulaugsepoctnovdec" );
  int month = shortMonthNames.indexOf( data.mid( pos, 3 ).toLower() );
  if ( month == -1 ) return start;
  month = month / 3 + 1;
  pos += 4;
  const int year = data.mid( pos, 4 ).toInt( &ok );
  if ( !ok ) return start;
  pos += 5;
  const int hours = data.mid( pos, 2 ).toInt( &ok );
  if ( !ok ) return start;
  pos += 3;
  const int minutes = data.mid( pos, 2 ).toInt( &ok );
  if ( !ok ) return start;
  pos += 3;
  const int seconds = data.mid( pos, 2 ).toInt( &ok );
  if ( !ok ) return start;
  pos += 4;
  const int tzhh = data.mid( pos, 2 ).toInt( &ok );
  if ( !ok ) return start;
  pos += 2;
  const int tzmm = data.mid( pos, 2 ).toInt( &ok );
  if ( !ok ) return start;
  int tzsecs = tzhh*60*60 + tzmm*60;
  if ( data[pos - 3] == '-' ) tzsecs = -tzsecs;
  const QDate date( year, month, day );
  const QTime time( hours, minutes, seconds );
  dateTime = QDateTime( date, time, Qt::UTC );
  if ( !dateTime.isValid() ) return start;
  dateTime = dateTime.addSecs( -tzsecs );

  pos += 2;
  if ( data.length() <= pos || !quoted )
    return pos;
  if ( data[pos] == '"' )
    ++pos;
  return pos;
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
