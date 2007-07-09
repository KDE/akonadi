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

#include <QtCore/QString>

#include "fetchquery.h"

using namespace Akonadi;

bool FetchQuery::parse( const QByteArray &query )
{
  int start = 0;

  if ( query.toUpper().startsWith( "UID FETCH " ) ) {
    mIsUidFetch = true;
    start = 10;
  } else if ( query.toUpper().startsWith( "FETCH " ) ) {
    mIsUidFetch = false;
    start = 6;
  } else
    return false;


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

    QByteArray attributeString = leftover.mid( 1, leftover.length() - 2 );
    if ( attributeString.endsWith( ')' ) )
      attributeString.chop( 1 );
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

bool FetchQuery::hasAttributeType( Attribute::Type type ) const
{
  for ( int i = 0; i < mAttributes.count(); ++i ) {
    if ( mAttributes[ i ].type() == type )
      return true;
  }

  return false;
}

QList<QByteArray> FetchQuery::sequences() const
{
  return mSequences;
}

QList<FetchQuery::Attribute> FetchQuery::attributes() const
{
  return mAttributes;
}

FetchQuery::Type FetchQuery::type() const
{
  return mType;
}

bool FetchQuery::isUidFetch() const
{
  return mIsUidFetch;
}

void FetchQuery::dump() const
{
  QByteArray type, sequence;

  for ( int i = 0; i < mSequences.count(); ++i )
    sequence += '|' + mSequences[ i ];
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
  qDebug( "Sequence:  %s", sequence.constData() );
  qDebug( "Type:      %s", type.constData() );

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
  else if ( attribute == "UID" || attribute == "REMOTEID" ) {
    // ignore
  } else {
    mType = Custom;
    mName = QString::fromUtf8( attribute );
  }

  return true;
}

FetchQuery::Attribute::Type FetchQuery::Attribute::type() const
{
  return mType;
}

void FetchQuery::Attribute::dump() const
{
  QByteArray type;

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
  else if ( mType == Custom )
    type = mName.toUtf8();

  qDebug( "Attribute: %s", type.constData() );
}
