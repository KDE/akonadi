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

#include <QString>

#include "fetchquery.h"

using namespace Akonadi;

bool FetchQuery::parse( const QByteArray &query )
{
  int start = 6;
  int end = query.indexOf( ' ', start );

  if ( end == -1 )
    return false;

  const QByteArray sequence = query.mid( start, end - start );
  mSequences = sequence.split( ',' );

  const QByteArray leftover = query.mid( end + 1 );
  if ( leftover.toUpper().startsWith( "ALL" ) )
    mType = AllType;
  else if ( leftover.toUpper().startsWith( "FULL" ) )
    mType = FullType;
  else if ( leftover.toUpper().startsWith( "FAST" ) )
    mType = FastType;
  else if ( leftover.startsWith( '(' ) ) {
    mType = AttributeListType;

    const QByteArray attributeString = leftover.mid( 1, leftover.length() - 2 );
    QList<QByteArray> attributes = attributeString.split( ' ' );
    for ( int i = 0; i < attributes.count(); ++i ) {
      Attribute attribute;
      if ( !attribute.parse( attributes[ i ] ) )
        return false;

      mAttributes.append( attribute );
    }
  } else {
    mType = AttributeType;

    Attribute attribute;
    if ( !attribute.parse( leftover ) )
      return false;

    mAttributes.append( attribute );
  }

  return true;
}

QList<QByteArray> FetchQuery::normalizedSequences( const QList<QByteArray> &sequences )
{
  int min = -1, max = -1;
  bool hasMax = false;

#define FETCH_CHECK( str ) \
  if ( str == "*" ) { \
    hasMax = true; \
  } else { \
    int num = str.toInt(); \
    if ( min == -1 ) { \
      min = max = num; \
    } else { \
      if ( num < min ) \
        min = num; \
      else if ( num > max ) \
        max = num; \
    } \
  }


  for ( int i = 0; i < sequences.count(); ++i ) {
    if ( sequences[ i ].contains( ':' ) ) {
      const QList<QByteArray> pair = sequences[ i ].split( ':' );

      FETCH_CHECK( pair[ 0 ] );
      FETCH_CHECK( pair[ 1 ] );
    } else {
      FETCH_CHECK( sequences[ i ] );
    }
  }

  QList<QByteArray> retval;
  retval.append( QByteArray::number( min ) );
  if ( hasMax )
    retval.append( "*" );
  else
    retval.append( QByteArray::number( max ) );

  return retval;
}

bool FetchQuery::hasAttributeType( Attribute::Type type ) const
{
  for ( int i = 0; i < mAttributes.count(); ++i ) {
    if ( mAttributes[ i ].mType == type )
      return true;
  }

  return false;
}

void FetchQuery::dump()
{
  QString type, sequence;

  for ( int i = 0; i < mSequences.count(); ++i )
    sequence += "|" + QString::fromLatin1( mSequences[ i ] );
  sequence.append( "|" );

  if ( mType == AllType )
    type = "ALL";
  else if ( mType == FullType )
    type = "FULL";
  else if ( mType == FastType )
    type = "FAST";
  else if ( mType == AttributeType )
    type = "ATTRIBUTE";
  else if ( mType == AttributeListType )
    type = "ATTRIBUTES";

  qDebug( "Fetch:" );
  qDebug( "Sequence:  %s", qPrintable( sequence ) );
  qDebug( "Type:      %s", qPrintable( type ) );

  if ( mType == AttributeType )
    mAttributes.first().dump();

  if ( mType == AttributeListType ) {
    for ( int i = 0; i < mAttributes.count(); ++i )
      mAttributes[ i ].dump();
  }
}

bool FetchQuery::Attribute::parse( const QByteArray &attribute )
{
  if ( attribute.toUpper().startsWith( "ENVELOPE" ) )
    mType = Envelope;
  else if ( attribute.toUpper().startsWith( "FLAGS" ) )
    mType = Flags;
  else if ( attribute.toUpper().startsWith( "INTERNALDATE" ) )
    mType = InternalDate;
  else if ( attribute.toUpper().startsWith( "RCF822.HEADER" ) )
    mType = RFC822_Header;
  else if ( attribute.toUpper().startsWith( "RFC822.SIZE" ) )
    mType = RFC822_Size;
  else if ( attribute.toUpper().startsWith( "RFC822.TEXT" ) )
    mType = RFC822_Text;
  else if ( attribute.toUpper().startsWith( "RFC822" ) )
    mType = RFC822;
  else if ( attribute.toUpper().startsWith( "BODY.STRUCTURE" ) )
    mType = Body_Structure;
  else if ( attribute.toUpper().startsWith( "BODY" ) )
    mType = Body;
  else if ( attribute.toUpper().startsWith( "UID" ) )
    mType = Uid;

  return true;
}

void FetchQuery::Attribute::dump()
{
  QString type;

  if ( mType == Envelope )
    type = "ENVELOPE";
  else if ( mType == Flags )
    type = "FLAGS";
  else if ( mType == InternalDate )
    type = "INTERNALDATE";
  else if ( mType == RFC822_Header )
    type = "RFC822.HEADER";
  else if ( mType == RFC822_Size )
    type = "RFC822.SIZE";
  else if ( mType == RFC822_Text )
    type = "RFC822.TEXT";
  else if ( mType == RFC822 )
    type = "RFC822";
  else if ( mType == Body_Structure )
    type = "BODY.STRUCTURE";
  else if ( mType == Body )
    type = "BODY";
  else if ( mType == Uid )
    type = "UID";

  qDebug( "Attribute: %s", qPrintable( type ) );
}
