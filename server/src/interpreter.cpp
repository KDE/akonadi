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

#include "interpreter.h"

using namespace Akonadi;

SearchInterpreterItem::SearchInterpreterItem( const QString &key, const QString &comparator, const QString &pattern )
  : mKey( key ), mComparator( comparator ), mPattern( pattern ), mIsLeaf( true )
{
}

SearchInterpreterItem::SearchInterpreterItem( Relation relation )
  : mRelation( relation ), mIsLeaf( false )
{
}

void SearchInterpreterItem::setChildItems( const QList<SearchInterpreterItem*> items )
{
  mItems = items;
}

QList<SearchInterpreterItem*> SearchInterpreterItem::childItems() const
{
  return mItems;
}

SearchInterpreterItem::~SearchInterpreterItem()
{
  qDeleteAll( mItems );
  mItems.clear();
}

bool SearchInterpreterItem::isLeafItem() const
{
  return mIsLeaf;
}

SearchInterpreterItem::Relation SearchInterpreterItem::relation() const
{
  return mRelation;
}

QString SearchInterpreterItem::key() const
{
  return mKey;
}

QString SearchInterpreterItem::comparator() const
{
  return mComparator;
}

QString SearchInterpreterItem::pattern() const
{
  return mPattern;
}

QString SearchInterpreterItem::dump() const
{
  if ( mIsLeaf ) {
    return QString( "(%1 %2 %3)" ).arg( mKey, mComparator, mPattern );
  } else {
    QString text = "(";
    text += ( mRelation == And ? "&" : "|" );
    for ( int i = 0; i < mItems.count(); ++i )
      text += mItems[ i ]->dump();
    text += QLatin1Char( ')' );

    return text;
  }
}

SearchInterpreterItem* SearchParser::parse( const QString &query ) const
{
  QString pattern( query.trimmed() );

  if ( pattern.isEmpty() ) {
    qDebug( "empty search pattern passed!" );
    return 0;
  }

  const QStringList tokens = tokenize( pattern );

  QStringListIterator it( tokens );

  return parseInternal( it );
}

QStringListIterator SearchParser::balanced( QStringListIterator _it ) const
{
  QStringListIterator it( _it );

  int stack = 0;
  while ( it.hasNext() ) {
    const QString token = it.next();
    if ( token[ 0 ] == '(' )
      stack++;
    else if ( token[ 0 ] == ')' ) {
      stack--;

      if ( stack == 0 ) {
        return it;
      }
    }
  }

  return _it;
}

QStringList SearchParser::tokenize( const QString &query ) const
{
  QString pattern( query.trimmed() );

  QStringList tokens;
  QString token;

  bool escaped = false;
  bool inBrackets = false;
  int i = 0;
  while ( i < pattern.count() ) {
    if ( inBrackets ) {
      if ( escaped ) {
        escaped = false;
      } else {
        if ( pattern[ i ] == '\\' ) {
          escaped = true;
          ++i;
          continue;
        }

        if ( pattern[ i ] == '\'' ) {
          inBrackets = false;
          ++i;
          continue;
        }
      }
    } else {
      if ( pattern[ i ] == '\'' ) {
        inBrackets = true;
        ++i;
        continue;
      }

      if ( pattern[ i ] == '(' ) {
        if ( !token.isEmpty() )
          tokens.append( token );

        tokens.append( "(" );
        token = QString();

        ++i;
        while ( pattern[ i ].isSpace() )
          ++i;

        continue;
      }

      if ( pattern[ i ] == ')' ) {
        if ( !token.isEmpty() )
          tokens.append( token );

        tokens.append( ")" );
        token = QString();

        ++i;
        while ( pattern[ i ].isSpace() )
          ++i;

        continue;
      }

      // we found a separator
      if ( pattern[ i ].isSpace() ) {
        tokens.append( token );
        token = QString();

        while ( pattern[ i ].isSpace() )
          ++i;

        continue;
      }
    }

    token.append( pattern[ i ] );
    ++i;
  }

  if ( !token.isEmpty() )
    tokens.append( token );

  return tokens;
}

SearchInterpreterItem* SearchParser::parseInternal( QStringListIterator &it ) const
{
  if ( !it.hasNext() )
    return 0;

  QString token = it.next();
  if ( token == QLatin1String( "&" ) || token == QLatin1String( "|" ) ) {
    /**
     * We have a term like: (&( ... )( ... )( ... ))
     */
    QList<SearchInterpreterItem*> childItems;

    const QString operatorSign = token;

    do {
      QStringListIterator nextIt = balanced( it );
      if ( !nextIt.hasNext() ) {
        qDeleteAll( childItems );
        return 0;
      }
      token = nextIt.next();

      if ( !nextIt.hasPrevious() ) {
        qDeleteAll( childItems );
        return 0;
      }
      nextIt.previous();

      SearchInterpreterItem *item = parseInternal( it );
      if ( item == 0 ) {
        qDeleteAll( childItems );
        return 0;
      }

      childItems.append( item );

      if ( token[ 0 ] != '(' )
        break;

      it = nextIt;
    } while ( true );

    SearchInterpreterItem *item = 0;

    if ( operatorSign == QLatin1String( "&" ) )
      item = new SearchInterpreterItem( SearchInterpreterItem::And );
    else
      item = new SearchInterpreterItem( SearchInterpreterItem::Or );

    item->setChildItems( childItems );

    return item;
  } else if ( token[ 0 ] == '(' ) {
    /**
     * We have a term like: ( ... )
     */
    return parseInternal( it );
  } else {
    /**
     * We have a term like: a = b
     */
    const QString key = token;

    if ( !it.hasNext() )
      return 0;

    const QString comparator = it.next();

    if ( !it.hasNext() )
      return 0;

    const QString pattern = it.next();

    return new SearchInterpreterItem( key, comparator, pattern );
  }

  return 0;
}
