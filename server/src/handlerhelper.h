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

#ifndef AKONADIHANDLERHELPER_H
#define AKONADIHANDLERHELPER_H

#include <QByteArray>
#include <QList>

namespace Akonadi {

class HandlerHelper
{
  public:
    static QList<QByteArray> splitLine( const QByteArray &line );
    /**
      Copied from libakonadi/imapparser.h!
      @todo Share this stuff somehow!
    */
    static int parseQuotedString( const QByteArray &data, QByteArray &result, int start = 0 );
    static int stripLeadingSpaces( const QByteArray &data, int start = 0 );
};

}

#endif
