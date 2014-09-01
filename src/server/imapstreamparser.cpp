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

#include "imapstreamparser.h"
#include "response.h"
#include "tracer.h"

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <ctype.h>
#include <QtNetwork/QLocalSocket>
#include <QIODevice>

using namespace Akonadi;
using namespace Akonadi::Server;

ImapStreamParser::ImapStreamParser(QIODevice *socket)
    : m_socket(socket)
    , m_position(0)
    , m_literalSize(0)
    , m_peeking(false)
    , m_timeout(30 * 1000)
{
}

ImapStreamParser::~ImapStreamParser()
{
}

void ImapStreamParser::setWaitTimeout(int msecs)
{
    m_timeout = msecs;
}

QString ImapStreamParser::readUtf8String()
{
    QByteArray tmp;
    tmp = readString();
    QString result = QString::fromUtf8(tmp);
    return result;
}

QByteArray ImapStreamParser::readString()
{
    QByteArray result;
    if (!waitForMoreData(m_data.length() == 0)) {
        throw ImapParserException("Unable to read more data");
    }
    stripLeadingSpaces();
    if (!waitForMoreData(m_position >= m_data.length())) {
        throw ImapParserException("Unable to read more data");
    }

    // literal string
    // TODO: error handling
    if (hasLiteral()) {
        while (!atLiteralEnd()) {
            result += readLiteralPart();
        }
        return result;
    }

    // quoted string
    return parseQuotedString();
}

QByteArray ImapStreamParser::peekString()
{
    m_peeking = true;
    int pos = m_position;
    const QByteArray string = readString();
    m_position = pos;
    m_peeking = false;
    return string;
}

bool ImapStreamParser::hasString()
{
    if (!waitForMoreData(m_position >= m_data.length())) {
        throw ImapParserException("Unable to read more data");
    }
    int savedPos = m_position ;
    stripLeadingSpaces();
    int pos = m_position;
    m_position = savedPos;
    if (m_data[pos] == '{') {
        return true; //literal string
    }
    if (m_data[pos] == '"') {
        return true; //quoted string
    }
    if (m_data[pos] != ' ' &&
        m_data[pos] != '(' &&
        m_data[pos] != ')' &&
        m_data[pos] != '[' &&
        m_data[pos] != ']' &&
        m_data[pos] != '\n' &&
        m_data[pos] != '\r') {
        return true;  //unquoted string
    }

    return false; //something else, not a string
}

bool ImapStreamParser::hasLiteral(bool requestData)
{
    if (!waitForMoreData(m_position >= m_data.length())) {
        throw ImapParserException("Unable to read more data");
    }
    int savedPos = m_position;
    stripLeadingSpaces();
    if (m_data[m_position] == '{') {
        int end = -1;
        do {
            end = m_data.indexOf('}', m_position);
            if (!waitForMoreData(end == -1)) {
                throw ImapParserException("Unable to read more data");
            }
        } while (end == -1);
        Q_ASSERT(end > m_position);
        m_literalSize = m_data.mid(m_position + 1, end - m_position - 1).toInt();
        // strip CRLF
        m_position = end + 1;

        //IMAP inconsistency. IMAP always expects CRLF, but akonadi uses only LF.
        if (m_position < m_data.length() && m_data[m_position] == '\n') {
            ++m_position;
        }

        if (m_literalSize >= 0 && requestData) {
            sendContinuationResponse(m_literalSize);
        }
        return true;
    } else {
        m_position = savedPos;
        return false;
    }
}

bool ImapStreamParser::atLiteralEnd() const
{
    return (m_literalSize == 0);
}

qint64 ImapStreamParser::remainingLiteralSize()
{
    return m_literalSize;
}

QByteArray ImapStreamParser::readLiteralPart()
{
    static qint64 maxLiteralPartSize = 4096;
    int size = qMin(maxLiteralPartSize, m_literalSize);

    if (!waitForMoreData(m_data.length() < m_position + size)) {
        throw ImapParserException("Unable to read more data");
    }

    if (m_data.length() < m_position + size) {   // Still not enough data
        // Take what's already there
        size = m_data.length() - m_position;
    }

    QByteArray result = m_data.mid(m_position, size);
    m_position += size;
    m_literalSize -= size;
    Q_ASSERT(m_literalSize >= 0);
    if (!m_peeking) {
        m_data = m_data.right(m_data.size() - m_position);
        m_position = 0;
    }
    return result;
}

bool ImapStreamParser::hasSequenceSet()
{
    if (!waitForMoreData(m_position >= m_data.length())) {
        throw ImapParserException("Unable to read more data");
    }
    int savedPos = m_position;
    stripLeadingSpaces();
    int pos = m_position;
    m_position = savedPos;

    if (m_data[pos] == '*' ||  m_data[pos] == ':' || isdigit(m_data[pos])) {
        return true;
    }

    return false;
}

ImapSet ImapStreamParser::readSequenceSet()
{
    ImapSet result;
    if (!waitForMoreData(m_data.length() == 0)) {
        throw ImapParserException("Unable to read more data");
    }
    stripLeadingSpaces();
    qint64 value = -1, lower = -1, upper = -1;
    Q_FOREVER {
        if (!waitForMoreData(m_data.length() <= m_position)) {
            upper = value;
            if (lower < 0) {
                lower = value;
            }
            if (lower >= 0 && upper >= 0) {
                result.add(ImapInterval(lower, upper));
            }
            return result;
        }

        if (m_data[m_position] == '*') {
            value = 0;
        } else if (m_data[m_position] == ':') {
            lower = value;
        } else if (isdigit(m_data[m_position])) {
            bool ok = false;
            value = readNumber(&ok);
            Q_ASSERT(ok);   // TODO handle error
            --m_position;
        } else {
            upper = value;
            if (lower < 0) {
                lower = value;
            }
            result.add(ImapInterval(lower, upper));
            lower = -1;
            upper = -1;
            value = -1;
            if (m_data[m_position] != ',') {
                return result;
            }
        }
        ++m_position;
    }
}

bool ImapStreamParser::hasList()
{
    if (!waitForMoreData(m_position >= m_data.length())) {
        throw ImapParserException("Unable to read more data");
    }
    int savedPos = m_position;
    stripLeadingSpaces();
    if (!waitForMoreData(m_position >= m_data.length())) {
        throw ImapParserException("Unable to read more data");
    }
    int pos = m_position;
    m_position = savedPos;
    if (m_data[pos] == '(') {
        return true;
    }

    return false;
}

void ImapStreamParser::beginList()
{
    if (!waitForMoreData(m_position >= m_data.length())) {
        throw ImapParserException("Unable to read more data");
    }
    stripLeadingSpaces();
    if (!waitForMoreData(m_position >= m_data.length())) {
        throw ImapParserException("Unable to read more data");
    }
    if (m_data[m_position] != '(') {
        throw ImapParserException("Stream not at a beginning of a list");
    }
    ++m_position;
    return;
}

bool ImapStreamParser::atListEnd()
{
    if (!waitForMoreData(m_position >= m_data.length())) {
        throw ImapParserException("Unable to read more data");
    }
    int savedPos = m_position;
    stripLeadingSpaces();
    int pos = m_position;
    m_position = savedPos;
    if (m_data[pos] == ')') {
        m_position = pos + 1;
        return true;
    }

    return false;
}

QList<QByteArray> ImapStreamParser::readParenthesizedList()
{
    QList<QByteArray> result;
    if (!waitForMoreData(m_data.length() <= m_position)) {
        throw ImapParserException("Unable to read more data");
    }

    stripLeadingSpaces();
    if (m_data[m_position] != '(') {
        return result; //no list found
    }

    bool concatToLast = false;
    int count = 0;
    int sublistbegin = m_position;
    int i = m_position + 1;
    Q_FOREVER {
        if (!waitForMoreData(m_data.length() <= i)) {
            m_position = i;
            throw ImapParserException("Unable to read more data");
        }
        if (m_data[i] == '(') {
            ++count;
            if (count == 1) {
                sublistbegin = i;
            }
            ++i;
            continue;
        }
        if (m_data[i] == ')') {
            if (count <= 0) {
                m_position = i + 1;
                return result;
            }
            if (count == 1) {
                result.append(m_data.mid(sublistbegin, i - sublistbegin + 1));
            }
            --count;
            ++i;
            continue;
        }
        if (m_data[i] == ' ') {
            ++i;
            continue;
        }
        if (m_data.at(i) == '"') {
            if (count > 0) {
                m_position = i;
                parseQuotedString();
                i = m_position;
                continue;
            }
        }
        if (m_data[i] == '[') {
            concatToLast = true;
            result.last() += '[';
            ++i;
            continue;
        }
        if (m_data[i] == ']') {
            concatToLast = false;
            result.last() += ']';
            ++i;
            continue;
        }
        if (count == 0) {
            m_position = i;
            QByteArray ba;
            if (hasLiteral()) {
                while (!atLiteralEnd()) {
                    ba += readLiteralPart();
                }
            } else {
                ba = readString();
            }

            // We might sometime get some unwanted CRLF, but we're still not at the end
            // of the list, would make further string reads fail so eat the CRLFs.
            while ((m_position < m_data.size()) &&
                   (m_data[m_position] == '\r' || m_data[m_position] == '\n')) {
                m_position++;
            }

            i = m_position - 1;
            if (concatToLast) {
                result.last() += ba;
            } else {
                result.append(ba);
            }
        }
        ++i;
    }

    throw ImapParserException("Something went very very wrong!");
}

QByteRef ImapStreamParser::readChar()
{
    if (!waitForMoreData(m_position >= m_data.length())) {
        throw ImapParserException("Unable to read more data");
    }
    m_position++;
    return m_data[m_position - 1];
}

QDateTime ImapStreamParser::readDateTime()
{
    // Syntax:
    // date-time      = DQUOTE date-day-fixed "-" date-month "-" date-year
    //                  SP time SP zone DQUOTE
    // date-day-fixed = (SP DIGIT) / 2DIGIT
    //                    ; Fixed-format version of date-day
    // date-month     = "Jan" / "Feb" / "Mar" / "Apr" / "May" / "Jun" /
    //                  "Jul" / "Aug" / "Sep" / "Oct" / "Nov" / "Dec"
    // date-year      = 4DIGIT
    // time           = 2DIGIT ":" 2DIGIT ":" 2DIGIT
    //                    ; Hours minutes seconds
    // zone           = ("+" / "-") 4DIGIT
    //                    ; Signed four-digit value of hhmm representing
    //                    ; hours and minutes east of Greenwich (that is,
    //                    ; the amount that the given time differs from
    //                    ; Universal Time).  Subtracting the timezone
    //                    ; from the given time will give the UT form.
    //                    ; The Universal Time zone is "+0000".
    // Example : "28-May-2006 01:03:35 +0200"
    // Position: 0123456789012345678901234567
    //                     1         2

    int savedPos = m_position;
    if (!waitForMoreData(m_data.length() == 0)) {
        throw ImapParserException("Unable to read more data");
    }
    stripLeadingSpaces();

    bool quoted = false;
    if (m_data[m_position] == '"') {
        quoted = true;
        ++m_position;

        if (m_data.length() <= m_position + 26) {
            m_position = savedPos;
            return QDateTime();
        }
    } else {
        if (m_data.length() < m_position + 26) {
            m_position = savedPos;
            return QDateTime();
        }
    }

    bool ok = true;
    const int day = (m_data[m_position] == ' ' ? m_data[m_position + 1] - '0'  // single digit day
                     : m_data.mid(m_position, 2).toInt(&ok));
    if (!ok) {
        m_position = savedPos;
        return QDateTime();
    }
    m_position += 3;
    const QByteArray shortMonthNames("janfebmaraprmayjunjulaugsepoctnovdec");
    int month = shortMonthNames.indexOf(m_data.mid(m_position, 3).toLower());
    if (month == -1) {
        m_position = savedPos;
        return QDateTime();
    }
    month = month / 3 + 1;
    m_position += 4;
    const int year = m_data.mid(m_position, 4).toInt(&ok);
    if (!ok) {
        m_position = savedPos;
        return QDateTime();
    }
    m_position += 5;
    const int hours = m_data.mid(m_position, 2).toInt(&ok);
    if (!ok) {
        m_position = savedPos;
        return QDateTime();
    }
    m_position += 3;
    const int minutes = m_data.mid(m_position, 2).toInt(&ok);
    if (!ok) {
        m_position = savedPos;
        return QDateTime();
    }
    m_position += 3;
    const int seconds = m_data.mid(m_position, 2).toInt(&ok);
    if (!ok) {
        m_position = savedPos;
        return QDateTime();
    }
    m_position += 4;
    const int tzhh = m_data.mid(m_position, 2).toInt(&ok);
    if (!ok) {
        m_position = savedPos;
        return QDateTime();
    }
    m_position += 2;
    const int tzmm = m_data.mid(m_position, 2).toInt(&ok);
    if (!ok) {
        m_position = savedPos;
        return QDateTime();
    }
    int tzsecs = tzhh * 60 * 60 + tzmm * 60;
    if (m_data[m_position - 3] == '-') {
        tzsecs = -tzsecs;
    }
    const QDate date(year, month, day);
    const QTime time(hours, minutes, seconds);
    QDateTime dateTime;
    dateTime = QDateTime(date, time, Qt::UTC);
    if (!dateTime.isValid()) {
        m_position = savedPos;
        return QDateTime();
    }
    dateTime = dateTime.addSecs(-tzsecs);

    m_position += 2;
    if (m_data.length() <= m_position || !quoted) {
        return dateTime;
    }
    if (m_data[m_position] == '"') {
        ++m_position;
    }
    return dateTime;
}

bool ImapStreamParser::hasDateTime()
{
    int savedPos = m_position;
    QDateTime dateTime = readDateTime();
    m_position = savedPos;
    return !dateTime.isNull();
}

QByteArray ImapStreamParser::parseQuotedString()
{
    QByteArray result;
    if (!waitForMoreData(m_data.length() == 0)) {
        throw ImapParserException("Unable to read more data");
    }
    stripLeadingSpaces();
    int end = m_position;
    result.clear();
    if (!waitForMoreData(m_position >= m_data.length())) {
        throw ImapParserException("Unable to read more data");
    }
    if (!waitForMoreData(m_position >= m_data.length())) {
        throw ImapParserException("Unable to read more data");
    }

    bool foundSlash = false;
    // quoted string
    if (m_data[m_position] == '"') {
        ++m_position;
        int i = m_position;
        Q_FOREVER {
            if (!waitForMoreData(m_data.length() <= i)) {
                m_position = i;
                throw ImapParserException("Unable to read more data");
            }

            if (foundSlash) {
                foundSlash = false;
                if (m_data[i] == 'r') {
                    result += '\r';
                } else if (m_data[i] == 'n') {
                    result += '\n';
                } else if (m_data[i] == '\\') {
                    result += '\\';
                } else if (m_data[i] == '\"') {
                    result += '\"';
                } else {
                    throw ImapParserException("Unexpected '\\' in quoted string");
                }
                ++i;
                continue;
            }

            if (m_data[i] == '\\') {
                foundSlash = true;
                ++i;
                continue;
            }

            if (m_data[i] == '"') {
                end = i + 1; // skip the '"'
                break;
            }

            result += m_data[i];

            ++i;
        }
    } else {
        // unquoted string
        bool reachedInputEnd = true;
        int i = m_position;
        Q_FOREVER {
            if (!waitForMoreData(m_data.length() <= i)) {
                m_position = i;
                throw ImapParserException("Unable to read more data");
            }
            // unlike in the copy in KIMAP we do not want to consider [] brackets as separators, breaks payload version parsing
            // if that ever gets fixed we can re-add them here, see svn revision 937879
            if (m_data[i] == ' ' || m_data[i] == '(' || m_data[i] == ')' || m_data[i] == '\n' || m_data[i] == '\r' || m_data[i] == '"') {
                end = i;
                reachedInputEnd = false;
                break;
            }
            if (m_data[i] == '\\') {
                foundSlash = true;
            }
            i++;
        }
        if (reachedInputEnd) {   //FIXME: how can it get here?
            end = m_data.length();
        }

        result = m_data.mid(m_position, end - m_position);

        // transform unquoted NIL
        if (result == "NIL") {
            result.clear();
        }

        // strip quotes
        if (foundSlash) {
            while (result.contains("\\\"")) {
                result.replace("\\\"", "\"");
            }
            while (result.contains("\\\\")) {
                result.replace("\\\\", "\\");
            }
        }
    }

    m_position = end;
    return result;
}

qint64 ImapStreamParser::readNumber(bool *ok)
{
    qint64  result;
    if (ok) {
        *ok = false;
    }
    if (!waitForMoreData(m_data.length() == 0)) {
        throw ImapParserException("Unable to read more data");
    }
    stripLeadingSpaces();
    if (!waitForMoreData(m_position >= m_data.length())) {
        throw ImapParserException("Unable to read more data");
    }
    if (m_position >= m_data.length()) {
        throw ImapParserException("Unable to read more data");
    }
    int i = m_position;
    Q_FOREVER {
        if (!waitForMoreData(m_data.length() <= i)) {
            m_position = i;
            throw ImapParserException("Unable to read more data");
        }
        if (!isdigit(m_data.at(i))) {
            break;
        }
        ++i;
    }
    const QByteArray tmp = m_data.mid(m_position, i - m_position);
    bool success = false;
    result = tmp.toLongLong(&success);
    if (ok) {
        *ok = success;
    } else if (!success) {
        throw ImapParserException("Unable to parse number");
    }
    m_position = i;
    return result;
}

void ImapStreamParser::stripLeadingSpaces()
{
    for (int i = m_position; i < m_data.length(); ++i) {
        if (m_data[i] != ' ') {
            m_position = i;
            return;
        }
    }
    m_position = m_data.length();
}

bool ImapStreamParser::waitForMoreData(bool wait)
{
    if (wait) {
        if (m_socket->bytesAvailable() > 0 ||
            m_socket->waitForReadyRead(m_timeout)) {
            m_data.append(m_socket->readAll());
        } else {
            return false;
        }
    }
    return true;
}

void ImapStreamParser::setData(const QByteArray &data)
{
    m_data = data;
}

QByteArray ImapStreamParser::readRemainingData()
{
    return m_data.mid(m_position);
}

bool ImapStreamParser::atCommandEnd()
{
    if (!waitForMoreData(m_position >= m_data.length())) {
        throw ImapParserException("Unable to read more data");
    }
    int savedPos = m_position;
    stripLeadingSpaces();
    if (m_data[m_position] == '\n' || m_data[m_position] == '\r') {
        if (m_position < m_data.length() && m_data[m_position] == '\r') {
            ++m_position;
        }
        if (m_position < m_data.length() && m_data[m_position] == '\n') {
            ++m_position;
        }
        // We'd better empty m_data from time to time before it grows out of control
        if (!m_peeking) {
            m_data = m_data.right(m_data.size() - m_position);
            m_position = 0;
        }
        return true; //command end
    }
    m_position = savedPos;
    return false; //something else
}

QByteArray ImapStreamParser::readUntilCommandEnd()
{
    QByteArray result;
    int i = m_position;
    int paranthesisBalance = 0;
    bool inQuotedString = false;
    Q_FOREVER {
        if (!waitForMoreData(m_data.length() <= i)) {
            m_position = i;
            throw ImapParserException("Unable to read more data");
        }
        if (!inQuotedString && m_data[i] == '{') {
            m_position = i - 1;
            hasLiteral(); //init literal size
            result.append(m_data.mid(i - 1, m_position - i + 1));
            while (!atLiteralEnd()) {
                result.append(readLiteralPart());
            }
            i = m_position;
            do {
                result.append(m_data[i]);
                ++i;
            } while (m_data[i] == ' ' || m_data[i] == '\n' || m_data[i] == '\r');
        }

        if (!inQuotedString && m_data[i] == '(') {
            paranthesisBalance++;
        }
        if (!inQuotedString && m_data[i] == ')') {
            paranthesisBalance--;
        }
        result.append(m_data[i]);

        if (m_data[i] == '"') {
            if (m_data[i - 1] != '\\') {
                inQuotedString = !inQuotedString;
            }
        }

        if ((i == m_data.length() && paranthesisBalance == 0) || m_data[i] == '\n'  || m_data[i] == '\r') {
            // Make sure we return \r\n and not just \r
            if (m_data[i] == '\r' && m_data[i + 1] == '\n') {
                ++i;
                result.append('\n');
            }
            break; //command end
        }
        ++i;
    }
    m_position = i + 1;
    // We'd better empty m_data from time to time before it grows out of control
    if (!m_peeking) {
        m_data = m_data.right(m_data.size() - m_position);
        m_position = 0;
    }
    return result;
}

void ImapStreamParser::skipCurrentCommand()
{
    int i = m_position;
    Q_FOREVER {
        if (!waitForMoreData(m_data.length() <= i)) {
            m_position = i;
            throw ImapParserException("Unable to read more data");
        }
        if (m_data[i] == '\n'  || m_data[i] == '\r') {
            break; //command end
        }
        ++i;
    }
    m_position = i + 1;
    // We'd better empty m_data from time to time before it grows out of control
    if (!m_peeking) {
        m_data = m_data.right(m_data.size() - m_position);
        m_position = 0;
    }
}

void ImapStreamParser::sendContinuationResponse(qint64 size)
{
    const QByteArray block = "+ Ready for literal data (expecting "
                             + QByteArray::number(size) + " bytes)\r\n";
    m_socket->write(block);
    m_socket->waitForBytesWritten(m_timeout);

    Tracer::self()->connectionOutput(m_tracerId, block);
}

void ImapStreamParser::insertData(const QByteArray &data)
{
    m_data = m_data.insert(m_position, data);
}

void ImapStreamParser::appendData(const QByteArray &data)
{
    m_data = m_data + data;
}

void ImapStreamParser::setTracerIdentifier(const QString &id)
{
    m_tracerId = id;
}
