/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_IMAPPARSER_H
#define AKONADI_IMAPPARSER_H

#include <kdepim_export.h>

#include <QtCore/QByteArray>
#include <QtCore/QList>

namespace Akonadi {

/**
  Helper functions to parse IMAP responses.
  @todo Not really Akonadi specific, move somewhere else.
*/
class AKONADI_EXPORT ImapParser
{
  public:
    /**
      Parses the next parenthesized list in @p data starting from @p start
      and puts the result into @p result. The number of used characters is
      returned.
      This does not recurse into sub-lists.
      @param data Source data.
      @param result The parsed list.
      @param start start parsing at this index.
    */
    static int parseParenthesizedList( const QByteArray &data, QList<QByteArray> &result, int start = 0 );

    /**
      Parse the next string in @p data (quoted or literal) starting from @p start
      and puts the result into @p result. The number of used characters is returned
      (this is not equal to result.length()!).
      @param data Source data.
      @param result Parsed string, quotation, literal marker, etc. are removed,
      'NIL' is transformed into an empty QByteArray.
      @param start start parsing at this index.
    */
    static int parseString( const QByteArray &data, QByteArray &result, int start = 0 );

    /**
      Parses the next quoted string from @p data starting at @p start and puts it into
      @p result. The number of parsed characters is returned (this is not equal to result.length()!).
      @param data Source data.
      @param result Parsed string, quotation is removed and 'NIL' is transformed to an empty QByteArray.
      @param start start parsing at this index.
    */
    static int parseQuotedString( const QByteArray &data, QByteArray &result, int start = 0 );

    /**
      Returns the number of leading espaces in @p data starting from @p start.
      @param data The source data.
      @param start start parsing from here.
    */
    static int stripLeadingSpaces( const QByteArray &data, int start = 0 );

    /**
      Returns the parentheses balance for the given data, considering quotes.
      @param data The source data.
      @param start start parsing from here.
    */
    static int parenthesesBalance( const QByteArray &data, int start = 0 );

    /**
      Joins a QByteArray list with the given separator.
      @param list The QByteArray list to join.
      @param separator The separator
    */
    static QByteArray join( const QList<QByteArray> &list, const QByteArray &separator );

    /**
      Same as parseString() and additional UTF-8 decoding of the result.
      @param data Source data.
      @param result Parsed string, quotation, literal marker, etc. are removed,
      'NIL' is transformed into an empty QString. UTF-8 decoding is applied..
      @param start start parsing at this index.
    */
    static int parseString( const QByteArray &data, QString &result, int start = 0 );

    /**
      Parses the next integer number from @p data starting at start and puts it into
      @p result. The number of characters parsed is returned (this is not the parsed result!).
      @param data Source data.
      @param result Parsed integer number, invalid of ok is false.
      @param ok Set to false if the parsing failed.
      @param start start parsing at this index.
    */
    static int parseNumber( const QByteArray &data, int &result, bool *ok = 0, int start = 0 );
};

}

#endif
