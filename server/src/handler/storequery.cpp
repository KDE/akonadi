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

#include "storequery.h"

using namespace Akonadi;

StoreQuery::StoreQuery()
  : mType( 0 )
{
}

bool StoreQuery::parse( const QByteArray &query )
{
  int start;
  if ( query.toUpper().startsWith( "STORE " ) )
    start = 6;

  int end = query.indexOf( ' ', start );

  const QByteArray sequences = query.mid( start, end - start );
  mSequences = sequences.split( ',' );

  start = end + 1;
  end = query.indexOf( ' ', start );

  const QByteArray subCommand = query.mid( start, end - start ).toUpper();
  if ( subCommand == "FLAGS" ) {
    mType = Replace;
  } else if ( subCommand == "FLAGS.SILENT" ) {
    mType = Replace | Silent;
  } else if ( subCommand == "+FLAGS" ) {
    mType = Add;
  } else if ( subCommand == "+FLAGS.SILENT" ) {
    mType = Add | Silent;
  } else if ( subCommand == "-FLAGS" ) {
    mType = Delete;
  } else if ( subCommand == "-FLAGS.SILENT" ) {
    mType = Delete | Silent;
  } else
    return false;

  const QByteArray leftover = query.mid( end + 1 );
  if ( !leftover[ 0 ] == '(' )
    return false;

  start = 1;
  end = leftover.indexOf( ')' );

  QList<QByteArray> flags = leftover.mid( start, end - start ).split( ' ' );
  for ( int i = 0; i < flags.count(); ++i ) {
    const QByteArray flag = flags[ i ].toUpper();

    if ( flag == "\\ANSWERED" )
      mFlags.append( "\\Answered" );
    else if ( flag == "\\FLAGGED" )
      mFlags.append( "\\Flagged" );
    else if ( flag == "\\DELETED" )
      mFlags.append( "\\Deleted" );
    else if ( flag == "\\SEEN" )
      mFlags.append( "\\Seen" );
    else if ( flag == "\\DRAFT" )
      mFlags.append( "\\Draft" );
  }

  return true;
}

void StoreQuery::dump()
{
  qDebug( "STORE:" );
  if ( mType & Replace ) qDebug( "Replace" );
  if ( mType & Add ) qDebug( "Add" );
  if ( mType & Delete ) qDebug( "Delete" );
  if ( mType & Silent ) qDebug( "Silent" );

  for ( int i = 0; i < mFlags.count(); ++i )
    qDebug( "%s", mFlags[ i ].data() );
}
