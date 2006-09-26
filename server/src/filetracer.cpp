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

#include <QtCore/QFile>

#include "filetracer.h"

using namespace Akonadi;

FileTracer::FileTracer( const QString &fileName )
{
  m_file = new QFile( fileName );
  m_file->open( QIODevice::WriteOnly | QIODevice::Unbuffered );
}

FileTracer::~FileTracer()
{
}

void FileTracer::beginConnection( const QString &identifier, const QString &msg )
{
  output( identifier, QString::fromLatin1( "begin_connection: %1" ).arg( msg ) );
}

void FileTracer::endConnection( const QString &identifier, const QString &msg )
{
  output( identifier, QString::fromLatin1( "end_connection: %1" ).arg( msg ) );
}

void FileTracer::connectionInput( const QString &identifier, const QString &msg )
{
  output( identifier, QString::fromLatin1( "input: %1" ).arg( msg ) );
}

void FileTracer::connectionOutput( const QString &identifier, const QString &msg )
{
  output( identifier, QString::fromLatin1( "output: %1" ).arg( msg ) );
}

void FileTracer::signal( const QString &signalName, const QString &msg )
{
  output( QLatin1String( "signal" ), QString::fromLatin1( "<%1> %2" ).arg( signalName, msg ) );
}

void FileTracer::warning( const QString &componentName, const QString &msg )
{
  output( QLatin1String( "warning" ), QString::fromLatin1( "<%1> %2" ).arg( componentName, msg ) );
}

void FileTracer::error( const QString &componentName, const QString &msg )
{
  output( QLatin1String( "error" ), QString::fromLatin1( "<%1> %2" ).arg( componentName, msg ) );
}

void FileTracer::output( const QString &id, const QString &msg )
{
  QString output = QString::fromLatin1( "%1: %2\r\n" ).arg( id, msg );
  m_file->write( output.toUtf8() );
}
