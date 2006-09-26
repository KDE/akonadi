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
#include "imapparser.h"
#include <QDebug>

using namespace Akonadi;

StoreQuery::StoreQuery()
  : mOperation( 0 ), mDataType( Flags ), mContinuationSize( -1 )
{
}

bool StoreQuery::parse( const QByteArray &query )
{
  int start = 6;
  if ( query.toUpper().startsWith( "STORE " ) ) {
    start = 6;
    mIsUidStore = false;
  } else if ( query.toUpper().startsWith( "UID STORE " ) ) {
    start = 10;
    mIsUidStore = true;
  }

  int end = query.indexOf( ' ', start );

  const QByteArray sequences = query.mid( start, end - start );
  mSequences = sequences.split( ',' );

  start = end + 1;
  end = query.indexOf( ' ', start );

  const QByteArray subCommand = query.mid( start, end - start ).toUpper();
  if ( subCommand == "FLAGS" ) {
    mOperation = Replace;
  } else if ( subCommand == "FLAGS.SILENT" ) {
    mOperation = Replace | Silent;
  } else if ( subCommand == "+FLAGS" ) {
    mOperation = Add;
  } else if ( subCommand == "+FLAGS.SILENT" ) {
    mOperation = Add | Silent;
  } else if ( subCommand == "-FLAGS" ) {
    mOperation = Delete;
  } else if ( subCommand == "-FLAGS.SILENT" ) {
    mOperation = Delete | Silent;
  } else if ( subCommand == "DATA" ) {
    mOperation = Replace | Silent;
    mDataType = Data;
  } else if ( subCommand == "COLLECTION" ) {
    mOperation = Replace | Silent;
    mDataType = Collection;
  }else
    return false;

  const QByteArray leftover = query.mid( end + 1 );
  if ( dataType() == Data ) {
    if ( !leftover[ 0 ] == '{' )
      return false;
    mContinuationSize = leftover.mid( 1, leftover.indexOf( '}' ) - 1 ).toInt();
    return true;
  }
  if ( dataType() == Collection ) {
    PIM::ImapParser::parseString( leftover, mCollection );
    if ( mCollection.isEmpty() )
      return false;
    return true;
  }
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
    else
      mFlags.append( flags[ i ] );
  }

  return true;
}

int StoreQuery::operation() const
{
  return mOperation;
}

QList<QByteArray> StoreQuery::flags() const
{
  return mFlags;
}

QList<QByteArray> StoreQuery::sequences() const
{
  return mSequences;
}

bool StoreQuery::isUidStore() const
{
  return mIsUidStore;
}

void StoreQuery::dump()
{
  qDebug( "STORE:" );
  if ( mOperation & Replace ) qDebug( "Replace" );
  if ( mOperation & Add ) qDebug( "Add" );
  if ( mOperation & Delete ) qDebug( "Delete" );
  if ( mOperation & Silent ) qDebug( "Silent" );

  for ( int i = 0; i < mFlags.count(); ++i )
    qDebug( "%s", mFlags[ i ].data() );
}

int Akonadi::StoreQuery::dataType() const
{
  return mDataType;
}

int Akonadi::StoreQuery::continuationSize() const
{
  return mContinuationSize;
}

QString Akonadi::StoreQuery::collection() const
{
  return mCollection;
}
