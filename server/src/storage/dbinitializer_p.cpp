/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *   Copyright (C) 2010 by Volker Krause <vkrause@kde.org>                 *
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

#include "storage/dbinitializer_p.h"

//BEGIN PostgreSQL

DbInitializerPostgreSql::DbInitializerPostgreSql(const QSqlDatabase& database, const QString& templateFile):
  DbInitializer(database, templateFile)
{
}

QString DbInitializerPostgreSql::sqlType(const QString& type) const
{
  if ( type == QLatin1String("qint64") )
    return QLatin1String( "int8" );
  if ( type == QLatin1String("QByteArray") )
    return QLatin1String("BYTEA");
  return DbInitializer::sqlType( type );
}

//END PostgreSQL



//BEGIN Virtuoso

DbInitializerVirtuoso::DbInitializerVirtuoso(const QSqlDatabase& database, const QString& templateFile):
  DbInitializer(database, templateFile)
{
}

QString DbInitializerVirtuoso::sqlType(const QString& type) const
{
  if ( type == QLatin1String("QString") )
    return QLatin1String( "VARCHAR(255)" );
  if (type == QLatin1String("QByteArray") )
    return QLatin1String("LONG VARCHAR");
  if ( type == QLatin1String( "bool" ) )
    return QLatin1String("CHAR");
  return DbInitializer::sqlType( type );
}

QString DbInitializerVirtuoso::sqlValue(const QString& type, const QString& value) const
{
  if ( type == QLatin1String( "bool" ) ) {
    if ( value == QLatin1String( "false" ) ) return QLatin1String( "0" );
    if ( value == QLatin1String( "true" ) ) return QLatin1String( "1" );
  }
  return DbInitializer::sqlValue( type, value );
}

//END Virtuoso

