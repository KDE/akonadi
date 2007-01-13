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

InterpreterItem::InterpreterItem( const QString &key, const QString &comparator, const QString &pattern )
  : mKey( key ), mComparator( comparator ), mPattern( pattern ), mIsLeaf( true )
{
}

InterpreterItem::InterpreterItem( Relation relation )
  : mRelation( relation ), mIsLeaf( false )
{
}

void InterpreterItem::setChildItems( const QList<InterpreterItem*> items )
{
  mItems = items;
}

QList<InterpreterItem*> InterpreterItem::childItems() const
{
  return mItems;
}

InterpreterItem::~InterpreterItem()
{
  qDeleteAll( mItems );
  mItems.clear();
}

bool InterpreterItem::isLeafItem() const
{
  return mIsLeaf;
}

InterpreterItem::Relation InterpreterItem::relation() const
{
  return mRelation;
}

QString InterpreterItem::key() const
{
  return mKey;
}

QString InterpreterItem::comparator() const
{
  return mComparator;
}

QString InterpreterItem::pattern() const
{
  return mPattern;
}

QString InterpreterItem::dump() const
{
  if ( mIsLeaf ) {
    return QString( "(%1 %2 %3)" ).arg( mKey, mComparator, mPattern );
  } else {
    QString text = "(";
    text += ( mRelation == And ? "&" : "|" );
    for ( int i = 0; i < mItems.count(); ++i )
      text += mItems[ i ]->dump();
    text += ")";

    return text;
  }
}

InterpreterItem* Parser::parse( const QString &query ) const
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

QStringListIterator Parser::balanced( QStringListIterator _it ) const
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

QStringList Parser::tokenize( const QString &query ) const
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

InterpreterItem* Parser::parseInternal( QStringListIterator &it ) const
{
  if ( !it.hasNext() )
    return 0;

  QString token = it.next();
  if ( token == QLatin1String( "&" ) || token == QLatin1String( "|" ) ) {
    /**
     * We have a term like: (&( ... )( ... )( ... ))
     */
    QList<InterpreterItem*> childItems;

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

      InterpreterItem *item = parseInternal( it );
      if ( item == 0 ) {
        qDeleteAll( childItems );
        return 0;
      }

      childItems.append( item );

      if ( token[ 0 ] != '(' )
        break;

      it = nextIt;
    } while ( true );

    InterpreterItem *item = 0;

    if ( operatorSign == QLatin1String( "&" ) )
      item = new InterpreterItem( InterpreterItem::And );
    else
      item = new InterpreterItem( InterpreterItem::Or );

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

    return new InterpreterItem( key, comparator, pattern );
  }

  return 0;
}
