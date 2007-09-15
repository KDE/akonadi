/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "akonadiprotocol_export.h"

#include <libakonadi/imapset.h>

#include <QtCore/QByteArray>
#include <QtCore/QList>

namespace Akonadi {

/**
  Parser for IMAP messages.
*/
class AKONADIPROTOCOL_EXPORT ImapParser
{
  public:
    /**
      Parses the next parenthesized list in @p data starting from @p start
      and puts the result into @p result. The number of used characters is
      returned.
      This does not recurse into sub-lists.
      @param data Source data.
      @param result The parsed list.
      @param start Start parsing at this index.
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
      @param start Start parsing at this index.
    */
    static int parseQuotedString( const QByteArray &data, QByteArray &result, int start = 0 );

    /**
      Returns the number of leading espaces in @p data starting from @p start.
      @param data The source data.
      @param start Start parsing at this index.
    */
    static int stripLeadingSpaces( const QByteArray &data, int start = 0 );

    /**
      Returns the parentheses balance for the given data, considering quotes.
      @param data The source data.
      @param start Start parsing at this index.
    */
    static int parenthesesBalance( const QByteArray &data, int start = 0 );

    /**
      Joins a QByteArray list with the given separator.
      @param list The QByteArray list to join.
      @param separator The separator.
    */
    static QByteArray join( const QList<QByteArray> &list, const QByteArray &separator );

    /**
    Joins a QByteArray set with the given separator.
    @param set The QByteArray set to join.
    @param separator The separator.
     */
    static QByteArray join( const QSet<QByteArray> &set, const QByteArray &separator );

    /**
      Same as parseString(), but with additional UTF-8 decoding of the result.
      @param data Source data.
      @param result Parsed string, quotation, literal marker, etc. are removed,
      'NIL' is transformed into an empty QString. UTF-8 decoding is applied..
      @param start Start parsing at this index.
    */
    static int parseString( const QByteArray &data, QString &result, int start = 0 );

    /**
      Parses the next integer number from @p data starting at start and puts it into
      @p result. The number of characters parsed is returned (this is not the parsed result!).
      @param data Source data.
      @param result Parsed integer number, invalid if ok is false.
      @param ok Set to false if the parsing failed.
      @param start Start parsing at this index.
    */
    static int parseNumber( const QByteArray &data, int &result, bool *ok = 0, int start = 0 );

    /**
      Quotes the given QByteArray.
      @param data Source data.
    */
    static QByteArray quote( const QByteArray &data );

    /**
      Parse an IMAP sequence set.
      @param data source data.
      @param result The parse sequence set.
      @param start start parsing at this index.
      @return end position of parsing.
    */
    static int parseSequenceSet( const QByteArray &data, ImapSet &result, int start = 0 );

    /**
      Parse an IMAP date/time value.
      @param data source data.
      @param dateTime The result date/time.
      @param start Start parsing at this index.
      @return end position of parsing.
    */
    static int parseDateTime( const QByteArray &data, QDateTime &dateTime, int start = 0 );


    /**
      Constructs a new IMAP parser.
    */
    ImapParser();

    /**
      Destroys an IMAP parser.
    */
    ~ImapParser();

    /**
      Parses the given line.
      @returns True if an IMAP message was parsed completely, false if more data is needed.
      @todo read from a QIODevice directly to avoid an extra line buffer
    */
    bool parseNextLine( const QByteArray &readBuffer );

    /**
      Returns the tag of the parsed message.
      Only valid if parseNextLine() returned true.
    */
    QByteArray tag() const;

    /**
      Return the raw data of the parsed IMAP message.
      Only valid if parseNextLine() returned true.
    */
    QByteArray data() const;

    /**
      Resets the internal state of the parser. Call before parsing
      a new IMAP message.
    */
    void reset();

    /**
      Returns true if the last parsed line contained a literal continuation,
      ie. readiness for receiving literal data needs to be indicated.
    */
    bool continuationStarted() const;

    /**
      Returns the expected size of liteal data.
    */
    int continuationSize() const;

  private:
    class Private;
    Private *const d;
};

}

#endif
