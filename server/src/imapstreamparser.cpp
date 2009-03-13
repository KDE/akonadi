/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2009 Andras Mantia <amantia@kde.org>

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

#include "imapstreamparser.h"
#include "response.h"
#include "tracer.h"

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <ctype.h>
#include <QtNetwork/QLocalSocket>
#include <QIODevice>

using namespace Akonadi;

ImapStreamParser::ImapStreamParser( QIODevice *socket )
{
  m_socket = socket;
  m_position = 0;
  m_literalSize = 0;
  m_continuationSize = 0;
}

ImapStreamParser::~ImapStreamParser()
{
}

QByteArray ImapStreamParser::tag()
{
  if (m_tag.isEmpty())
  {
    m_tag = readString();
  }
  return m_tag;
}

QString ImapStreamParser::readUtf8String()
{
  QByteArray tmp;
  tmp = readString();
  QString result = QString::fromUtf8( tmp );
  return result;
}


QByteArray ImapStreamParser::readString()
{
  QByteArray result;
  if ( !waitForMoreData( m_data.length() == 0 ) )
    throw ImapParserException("Unable to read more data");
  stripLeadingSpaces();
  if ( !waitForMoreData( m_position >= m_data.length() ) )
    throw ImapParserException("Unable to read more data");;

  // literal string
  // TODO: error handling
  if ( m_data[m_position] == '{' ) {
    int end = -1;
    do {
      end = m_data.indexOf( '}', m_position );
      if ( !waitForMoreData( end == -1 ) )
        throw ImapParserException("Unable to read more data");
    } while (end == -1);
    Q_ASSERT( end > m_position );
    int size = m_data.mid( m_position + 1, end - m_position - 1 ).toInt();

    // strip CRLF
    m_position = end + 1;

    if ( m_position < m_data.length() && m_data[m_position] == '\r' )
      ++m_position;
    if ( m_position < m_data.length() && m_data[m_position] == '\n' )
      ++m_position;


    end = m_position + size;
    m_continuationSize = end - m_data.length();
    if (m_continuationSize > 0)
      sendContinuationResponse();
    if ( !waitForMoreData( m_data.length() < end ) )
      throw ImapParserException("Unable to read more data");

    result = m_data.mid( m_position, end - m_position );
    m_position = end;
    return result;
  }

  // quoted string
  return parseQuotedString();
}

bool ImapStreamParser::hasString()
{
  if ( !waitForMoreData( m_position >= m_data.length() ) )
    throw ImapParserException("Unable to read more data");
  int savedPos = m_position;
  stripLeadingSpaces();
  int pos = m_position;
  m_position = savedPos;
  if ( m_data[pos] == '{' )
    return true; //literal string
  if (m_data[pos] == '"' )
    return true; //quoted string
  if ( m_data[pos] != ' ' &&
       m_data[pos] != '(' &&
       m_data[pos] != ')' &&
       m_data[pos] != '[' &&
       m_data[pos] != ']' &&
       m_data[pos] != '\n' &&
       m_data[pos] != '\r' )
    return true;  //unquoted string

  return false; //something else, not a string
}

bool ImapStreamParser::hasLiteral()
{
  if ( !waitForMoreData( m_position >= m_data.length() ) )
    throw ImapParserException("Unable to read more data");
  int savedPos = m_position;
  stripLeadingSpaces();
  if ( m_data[m_position] == '{' )
  {
    int end = -1;
    do {
      end = m_data.indexOf( '}', m_position );
      if ( !waitForMoreData( end == -1 ) )
        throw ImapParserException("Unable to read more data");
    } while (end == -1);
    Q_ASSERT( end > m_position );
    m_literalSize = m_data.mid( m_position + 1, end - m_position - 1 ).toInt();
    // strip CRLF
    m_position = end + 1;

    if ( m_position < m_data.length() && m_data[m_position] == '\r' )
      ++m_position;
    if ( m_position < m_data.length() && m_data[m_position] == '\n' )
      ++m_position;

    m_continuationSize = qMin(m_position + m_literalSize - m_data.length(), (qint64)4096);
    if (m_continuationSize > 0)
      sendContinuationResponse();
    return true;
  } else
  {
    m_position = savedPos;
    return false;
  }
}

bool ImapStreamParser::atLiteralEnd() const
{
  return (m_literalSize == 0);
}

QByteArray ImapStreamParser::readLiteralPart()
{
  static qint64 maxLiteralPartSize = 4096;
  int size = qMin(maxLiteralPartSize, m_literalSize);

  if ( !waitForMoreData( m_data.length() < m_position + size ) )
       throw ImapParserException("Unable to read more data");

  if ( m_data.length() < m_position + size ) { // Still not enough data
    // Take what's already there
    size = m_data.length() - m_position;
   }

  QByteArray result = m_data.mid(m_position, size);
  m_position += size;
  m_literalSize -= size;
  Q_ASSERT(m_literalSize >= 0);
  m_data = m_data.right( m_data.size() - m_position );
  m_position = 0;
  return result;
}

bool ImapStreamParser::hasSequenceSet()
{
  if ( !waitForMoreData( m_position >= m_data.length() ) )
    throw ImapParserException("Unable to read more data");
  int savedPos = m_position;
  stripLeadingSpaces();
  int pos = m_position;
  m_position = savedPos;

  if ( m_data[pos] == '*' ||  m_data[pos] == ':'|| isdigit( m_data[pos] )) {
    return true;
  }

  return false;
}

ImapSet ImapStreamParser::readSequenceSet()
{
  ImapSet result;
  if (! waitForMoreData( m_data.length() == 0 ) )
    throw ImapParserException("Unable to read more data");
  stripLeadingSpaces();
  qint64 value = -1, lower = -1, upper = -1;
  Q_FOREVER {
    if ( !waitForMoreData( m_data.length() <= m_position ) )
    {
      upper = value;
      if ( lower < 0 )
        lower = value;
      if ( lower >= 0 && upper >= 0 )
        result.add( ImapInterval( lower, upper ) );
      return result;
    }

    if ( m_data[m_position] == '*' ) {
      value = 0;
    } else if ( m_data[m_position] == ':' ) {
      lower = value;
    } else if ( isdigit( m_data[m_position] ) ) {
      bool ok = false;
      value = readNumber( &ok );
      Q_ASSERT( ok ); // TODO handle error
      --m_position;
    } else {
      upper = value;
      if ( lower < 0 )
        lower = value;
      result.add( ImapInterval( lower, upper ) );
      lower = -1;
      upper = -1;
      value = -1;
      if ( m_data[m_position] != ',' ) {
        return result;
      }
    }
    ++m_position;
  }
}

bool ImapStreamParser::hasList()
{
  if ( !waitForMoreData( m_position >= m_data.length() ) )
    throw ImapParserException("Unable to read more data");
  int savedPos = m_position;
  stripLeadingSpaces();
  int pos = m_position;
  m_position = savedPos;
  if ( m_data[pos] == '(' )
  {
    return true;
  }

  return false;
}

bool ImapStreamParser::atListEnd()
{
  if ( !waitForMoreData( m_position >= m_data.length() ) )
    throw ImapParserException("Unable to read more data");
  int savedPos = m_position;
  stripLeadingSpaces();
  int pos = m_position;
  m_position = savedPos;
  if ( m_data[pos] == ')' )
  {
    m_position = pos + 1;
    return true;
  }

  return false;
}

QList<QByteArray> ImapStreamParser::readParenthesizedList()
{
  QList<QByteArray> result;
  if (! waitForMoreData( m_data.length() <= m_position ) )
    throw ImapParserException("Unable to read more data");

  stripLeadingSpaces();
  if ( m_data[m_position] != '(' )
    return result; //no list found

  bool concatToLast = false;
  int count = 0;
  int sublistbegin = m_position;
  int i = m_position + 1;
  Q_FOREVER {
    if ( !waitForMoreData( m_data.length() <= i ) )
    {
      m_position = i;
      throw ImapParserException("Unable to read more data");
    }
    if ( m_data[i] == '(' ) {
      ++count;
      if ( count == 1 )
        sublistbegin = i;
      ++i;
      continue;
    }
    if ( m_data[i] == ')' ) {
      if ( count <= 0 ) {
        m_position = i + 1;
        return result;
      }
      if ( count == 1 )
        result.append( m_data.mid( sublistbegin, i - sublistbegin + 1 ) );
      --count;
      ++i;
      continue;
    }
    if ( m_data[i] == ' ' ) {
      ++i;
      continue;
    }
    if ( m_data[i] == '[' ) {
      concatToLast = true;
      result.last()+='[';
      ++i;
      continue;
    }
    if ( m_data[i] == ']' ) {
      concatToLast = false;
      result.last()+=']';
      ++i;
      continue;
    }
    if ( count == 0 ) {
      m_position = i;
      QByteArray ba;
      if (hasLiteral()) {
        while (!atLiteralEnd()) {
          ba+=readLiteralPart();
        }
      } else {
        ba = readString();
      }
      i = m_position - 1;
      if (concatToLast) {
        result.last() += ba;
      } else {
        result.append( ba );
      }
    }
    ++i;
  }

  throw ImapParserException( "Something went very very wrong!" );
}


QByteArray ImapStreamParser::parseQuotedString()
{
  QByteArray result;
  if (! waitForMoreData( m_data.length() == 0 ) )
    throw ImapParserException("Unable to read more data");
  stripLeadingSpaces();
  int end = m_position;
  result.clear();
  if ( !waitForMoreData( m_position >= m_data.length() ) )
    throw ImapParserException("Unable to read more data");
  if ( !waitForMoreData( m_position >= m_data.length() ) )
    throw ImapParserException("Unable to read more data");

  bool foundSlash = false;
  // quoted string
  if ( m_data[m_position] == '"' ) {
    ++m_position;
    int i = m_position;
    Q_FOREVER {
      if ( !waitForMoreData( m_data.length() <= i ) )
      {
        m_position = i;
        throw ImapParserException("Unable to read more data");
      }
      if ( m_data[i] == '\\' ) {
        i += 2;
        foundSlash = true;
        continue;
      }
      if ( m_data[i] == '"' ) {
        result = m_data.mid( m_position, i - m_position );
        end = i + 1; // skip the '"'
        break;
      }
      ++i;
    }
  }

  // unquoted string
  else {
    bool reachedInputEnd = true;
    int i = m_position;
    Q_FOREVER {
      if ( !waitForMoreData( m_data.length() <= i ) )
      {
        m_position = i;
        throw ImapParserException("Unable to read more data");
      }
      if ( m_data[i] == ' ' || m_data[i] == '(' || m_data[i] == ')' || m_data[i] == '[' || m_data[i] == ']' || m_data[i] == '\n' || m_data[i] == '\r' || m_data[i] == '"') {
        end = i;
        reachedInputEnd = false;
        break;
      }
      if (m_data[i] == '\\')
        foundSlash = true;
      i++;
    }
    if ( reachedInputEnd ) //FIXME: how can it get here?
      end = m_data.length();

    result = m_data.mid( m_position, end - m_position );

    // transform unquoted NIL
    if ( result == "NIL" )
      result.clear();
  }

  // strip quotes
  if ( foundSlash ) {
    while ( result.contains( "\\\"" ) )
      result.replace( "\\\"", "\"" );
    while ( result.contains( "\\\\" ) )
      result.replace( "\\\\", "\\" );
  }
  m_position = end;
  return result;
}

qint64 ImapStreamParser::readNumber( bool * ok )
{
  qint64  result;
  if ( ok )
    *ok = false;
  if (! waitForMoreData( m_data.length() == 0 ) )
    throw ImapParserException("Unable to read more data");
  stripLeadingSpaces();
  if ( !waitForMoreData( m_position >= m_data.length() ) )
    throw ImapParserException("Unable to read more data");
  if ( m_position >= m_data.length() )
    throw ImapParserException("Unable to read more data");
  int i = m_position;
  Q_FOREVER {
    if ( !waitForMoreData( m_data.length() <= i ) )
    {
      m_position = i;
      throw ImapParserException("Unable to read more data");
    }
    if ( !isdigit( m_data.at( i ) ) )
      break;
    ++i;
  }
  const QByteArray tmp = m_data.mid( m_position, i - m_position );
  result = tmp.toLongLong( ok );
  m_position = i;
  return result;
}

void ImapStreamParser::stripLeadingSpaces()
{
  for ( int i = m_position; i < m_data.length(); ++i ) {
    if ( m_data[i] != ' ' )
    {
      m_position = i;
      return;
    }
  }
  m_position = m_data.length();
}

bool ImapStreamParser::waitForMoreData( bool wait)
{
   if ( wait ) {
     if ( m_socket->bytesAvailable() > 0 ||
          m_socket->waitForReadyRead(30000) ) {
        m_data.append( m_socket->readAll() );
     } else
     {
       return false;
     }
   }
   return true;
}

void ImapStreamParser::setData( const QByteArray &data )
{
  m_data = data;
}

QByteArray ImapStreamParser::readRemainingData()
{
  return m_data.mid(m_position);
}

bool ImapStreamParser::atCommandEnd()
{
  if ( !waitForMoreData( m_position >= m_data.length() ) )
    throw ImapParserException("Unable to read more data");
  int savedPos = m_position;
  stripLeadingSpaces();
  if ( m_data[m_position] == '\n' || m_data[m_position] == '\r') {
    if ( m_position < m_data.length() && m_data[m_position] == '\r' )
      ++m_position;
    if ( m_position < m_data.length() && m_data[m_position] == '\n' )
      ++m_position;
    // We'd better empty m_data from time to time before it grows out of control
    m_data = m_data.right(m_data.size()-m_position);
    m_position = 0;
    return true; //command end
  }
  m_position = savedPos;
  return false; //something else
}

QByteArray ImapStreamParser::readUntilCommandEnd()
{
  QByteArray result;
  int i = m_position;
  int paranthesisBalance = 0;
  Q_FOREVER {
    if ( !waitForMoreData( m_data.length() <= i ) )
    {
      m_position = i;
      throw ImapParserException("Unable to read more data");
    }
    if ( m_data[i] == '{' )
    {
      m_position = i - 1;
      hasLiteral(); //init literal size
      result.append(m_data.mid(i-1, m_position - i +1));
      while (!atLiteralEnd())
      {
        result.append( readLiteralPart() );
      }
      i = m_position;
    }
    if ( m_data[i] == '(' )
      paranthesisBalance++;
    if ( m_data[i] == ')' )
      paranthesisBalance--;
    result.append( m_data[i]);
    if ( ( i == m_data.length() && paranthesisBalance == 0 ) || m_data[i] == '\n'  || m_data[i] == '\r')
      break; //command end
      ++i;
  }
  m_position = i + 1;
  return result;
}

void ImapStreamParser::sendContinuationResponse()
{
  QByteArray block = "+ Ready for literal data (expecting "
                   + QByteArray::number(  m_continuationSize ) + " bytes)\r\n";
  m_socket->write(block);
  m_socket->waitForBytesWritten(30000);

  QString identifier;
  identifier.sprintf( "%p", static_cast<void*>( this ) );
  Tracer::self()->connectionOutput( identifier, QString::fromUtf8( block ) );

}

void ImapStreamParser::insertData( const QByteArray& data)
{
  m_data = m_data.insert(m_position, data);
}
