/***************************************************************************
 *   Copyright (C) 2009 by Tobias Koenig <tokoe@kde.org>                   *
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

#include "search.h"

#include "akonadi.h"
#include "akonadiconnection.h"
#include "response.h"
#include "handlerhelper.h"
#include "nepomuksearch.h"
#include "imapstreamparser.h"

#include <QtCore/QStringList>

using namespace Akonadi;

Search::Search()
  : Handler()
{
}

Search::~Search()
{
}

bool Search::parseStream()
{
  const QByteArray queryString = m_streamParser->readString();
  if ( queryString.isEmpty() )
    return failureResponse( "No query specified" );

  NepomukSearch *service = new NepomukSearch;

  const QStringList uids = service->search( QString::fromUtf8( queryString ) );

  delete service;

  foreach ( const QString &uid, uids ) {
    Response response;
    response.setUntagged();
    response.setString( QString::fromLatin1( "SEARCH (UID %1)" ).arg( uid ) );
    emit responseAvailable( response );
  }

  Response response;
  response.setTag( tag() );
  response.setSuccess();
  response.setString( "SEARCH completed" );
  emit responseAvailable( response );

  deleteLater();
  return true;
}

#include "search.moc"
