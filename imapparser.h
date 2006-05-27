/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef PIM_IMAPPARSER_H
#define PIM_IMAPPARSER_H

#include <QByteArray>
#include <QList>

namespace PIM {

/**
  Helper functions to parse IMAP responses.
  @todo Not really Akonadi specific, move somewhere else.
*/
class ImapParser
{
  public:
    /**
      Returns a list of elements in the first parenthesized list found
      in the given data. This does not recurse into sub-lists.
      @param data Source data.
      @param start start parsing at this index.
    */
    static QList<QByteArray> parseParentheziedList( const QByteArray &data, int start = 0 );

    /**
      Parses the next quoted string from @p data starting at @p start and puts it into
      @p result. The number of parsed characters is returned (this is not equal to result.length()!).
      @param data Source data.
      @param result Parsed string, quotation is removed and 'NIL' is transformed to an empty QByteArray.
      @param start start parsing at this index.
    */
    static int parseQuotedString( const QByteArray &data, QByteArray &result, int start = 0 );

};

}

#endif
