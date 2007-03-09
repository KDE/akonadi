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

#include "handlerhelper.h"
#include "storage/datastore.h"
#include "storage/querybuilder.h"

using namespace Akonadi;

QList<QByteArray> HandlerHelper::splitLine( const QByteArray &line )
{
  QList<QByteArray> retval;

  int i, start = 0;
  bool escaped = false;
  for ( i = 0; i < line.count(); ++i ) {
    if ( line[ i ] == ' ' ) {
      if ( !escaped ) {
        retval.append( line.mid( start, i - start ) );
        start = i + 1;
      }
    } else if ( line[ i ] == '"' ) {
      if ( escaped ) {
        escaped = false;
      } else {
        escaped = true;
      }
    }
  }

  retval.append( line.mid( start, i - start ) );

  return retval;
}

QString Akonadi::HandlerHelper::normalizeCollectionName(const QString &name)
{
  QString collection = name;
  if ( collection.startsWith( QLatin1Char('/') )  )
    collection = collection.right( collection.length() - 1 );
  if ( collection.endsWith( QLatin1Char('/') ) )
    collection = collection.left( collection.length() - 1 );
  return collection;
}

Location HandlerHelper::collectionFromIdOrName(const QByteArray & id)
{
  // id is a number
  bool ok = false;
  int collectionId = id.toInt( &ok );
  if ( ok )
    return Location::retrieveById( collectionId );

  // id is a path
  QString path = QString::fromUtf8( id ); // ### should be UTF-7 for real IMAP compatibility
  path = normalizeCollectionName( path );

  const QStringList pathParts = path.split( QLatin1Char('/') );
  Location loc;
  foreach ( const QString part, pathParts ) {
    QueryBuilder<Location> qb;
    qb.addValueCondition( Location::nameColumn(), "=", part );
    if ( loc.isValid() )
      qb.addValueCondition( Location::parentIdColumn(), "=", loc.id() );
    else
      qb.addValueCondition( Location::parentIdColumn(), "=", 0 );
    if ( !qb.exec() )
      return Location();
    Location::List list = qb.result();
    if ( list.count() != 1 )
      return Location();
    loc = list.first();
  }
  return loc;
}

QString HandlerHelper::pathForCollection(const Location & loc)
{
  QStringList parts;
  Location current = loc;
  while ( current.isValid() ) {
    parts.prepend( current.name() );
    current = current.parent();
  }
  return parts.join( QLatin1String("/") );
}
