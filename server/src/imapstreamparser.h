/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2009 Andras Mantia <amantia@kde.org>

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

#ifndef AKONADI_IMAPSTREAMPARSER_P_H
#define AKONADI_IMAPSTREAMPARSER_P_H

#include "akonadiprotocolinternals_export.h"

#include "imapset_p.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QVarLengthArray>
#include <QtCore/QObject>

#include "exception.h"

AKONADI_EXCEPTION_MAKE_INSTANCE( ImapParserException );

class QIODevice;

namespace Akonadi {

/**
  Parser for IMAP messages that operates on a local socket stream.
*/
class AKONADIPROTOCOLINTERNALS_EXPORT ImapStreamParser
{
  public:
    /**
     * Construct the parser.
     * @param socket the local socket to work with.
     */
    ImapStreamParser( QIODevice* socket);

    /**
     * Destructor.
     */
    ~ImapStreamParser();

    /**
     * Gets the message tag. Further calls to it do nothing. This call might block.
     * @return the tag of the parsed message.
     */
    QByteArray tag();

    /**
     * Get a string from the message. If the upcoming data is not a quoted string, unquoted string or a literal,
     * the behavior is undefined. Use @ref hasString to be sure a string comes. This call might block.
     * @return the next string from the message as an utf8 string
     */
    QString readUtf8String();

    /**
     * Same as above, but without decoding it to utf8.
     * @return the next string from the message
     */
    QByteArray readString();

    /**
     * Get he next IMAP sequence set. If the upcoming data is not an IMAP sequence set,
     * the behavior is undefined. Use @ref hasSequenceSet to be sure a sequence set comes. This call might block.
     * @return the next IMAP sequence set.
     */
    ImapSet readSequenceSet();

    /**
     * Get he next parenthesized list. If the upcoming data is not a parenthesized list,
     * the behavior is undefined. Use @ref hasList to be sure a string comes. This call might block.
     * @return the next parenthesized list.
     */
    QList<QByteArray> readParenthesizedList();


    /**
     * Get the next data as a number. This call might block.
     * @param ok true if the data found was a number
     * @return the number
     */
    qint64 readNumber( bool * ok );

    /**
     * Check if the next data is a string or not. This call might block.
     * @return true if a string follows
     */
    bool hasString();

    /**
     * Check if the next data is a literal data or not. If a literal is found, the
     * internal position pointer is set to the beginning of the literal data.
     * This call might block.
     * @return true if a literal follows
     */
    bool hasLiteral();

    /**
     * Read the next literal sequence. This might or might not be the full data. Example code to read a literal would be:
     * @code
     * ImapStreamParser parser;
     *  ...
     * if (parser.hasLiteral())
     * {
     *   while (!parser.atLiteralEnd())
     *   {
     *      QByteArray data = parser.readLiteralPart();
     *      // do something with the data
     *   }
     * }
     * @endcode
     *
     * This call might block.
     *
     * @return part of a literal data
     */
    QByteArray readLiteralPart();

    /**
     * Check if the literal data end was reached. See @ref hasLiteral and @ref readLiteralPart .
     * @return true if the literal was completely read.
     */
    bool atLiteralEnd();

    /**
     * Check if the next data is an IMAP sequence set. This call might block.
     * @return true if an IMAP sequence set comes.
     */
    bool hasSequenceSet();

    /**
     * Check if the next data is a parenthesized list. This call might block.
     * @return true if a parenthesized list comes.
    */
    bool hasList();

     /**
     * Check if the next data is a parenthesized list end. This call might block.
     * @return true if a parenthesized list end.
      */
    bool atListEnd();

    /**
     * Check if the command end was reached
     * @return true if the end of command is reached
     */
    bool atCommandEnd();

    /**
     * Return everything that remained from the command.
     * @return the remaining command data
     */
    QByteArray readUntilCommandEnd();

    /**
     * Return all the data that was read from the socket, but not processed yet.
     * @return the remaining unprocessed data
     */
    QByteArray readRemainingData();

    void setData( QByteArray data );


  private:
    void stripLeadingSpaces();
    QByteArray parseQuotedString();

    /**
     * If the condition is true, wait for more data to be available from the socket.
     * If no data comes after a timeout (30000ms), it aborts and returns false.
     * @param wait the condition
     * @return true if more data is available
     */
    bool waitForMoreData( bool wait);

    /**
     * Inform the client to send more literal data.
     */
    void sendContinuationResponse();

    QIODevice *m_socket;
    QByteArray m_data;
    QByteArray m_tag;
    int m_position;
    qint64 m_literalSize;
    qint64 m_continuationSize;
};

}

#endif
