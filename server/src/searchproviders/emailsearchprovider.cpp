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

#include "emailsearchprovider.h"

using namespace Akonadi;

EmailSearchProvider::EmailSearchProvider()
{
}

EmailSearchProvider::~EmailSearchProvider()
{
}

QList<QByteArray> EmailSearchProvider::supportedMimeTypes() const
{
  QList<QByteArray> mimeTypes;

  mimeTypes.append( "message/rfc822" );
  mimeTypes.append( "message/news" );

  return mimeTypes;
}

QList<QByteArray> EmailSearchProvider::queryForUids( const QList<QByteArray> &searchCriteria ) const
{
  QList<QByteArray> dummy;

  if ( searchCriteria.contains( "foobar" ) ) {
    dummy.append( "12" );
    dummy.append( "22" );
    dummy.append( "34" );
    dummy.append( "108" );
  } else if ( searchCriteria.contains( "blaah" ) ) {
    dummy.append( "132" );
    dummy.append( "221" );
    dummy.append( "341" );
    dummy.append( "109" );
  }

  return dummy;
}

QList<QByteArray> EmailSearchProvider::queryForObjects( const QList<QByteArray> &searchCriteria ) const
{
  return QList<QByteArray>();
}

