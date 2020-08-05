/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "imapparser_p.h"

#include <QDateTime>

#include <ctype.h>

using namespace Akonadi;

class ImapParser::Private
{
public:
    QByteArray tagBuffer;
    QByteArray dataBuffer;
    int parenthesesCount;
    qint64 literalSize;
    bool continuation;

    // returns true if readBuffer contains a literal start and sets
    // parser state accordingly
    bool checkLiteralStart(const QByteArray &readBuffer, int pos = 0)
    {
        if (readBuffer.trimmed().endsWith('}')) {
            const int begin = readBuffer.lastIndexOf('{');
            const int end = readBuffer.lastIndexOf('}');

            // new literal in previous literal data block
            if (begin < pos) {
                return false;
            }

            // TODO error handling
            literalSize = readBuffer.mid(begin + 1, end - begin - 1).toLongLong();

            // empty literal
            if (literalSize == 0) {
                return false;
            }

            continuation = true;
            dataBuffer.reserve(dataBuffer.size() + static_cast<int>(literalSize) + 1);
            return true;
        }
        return false;
    }
};

namespace
{

template <typename T>
int parseParenthesizedListHelper(const QByteArray &data, T &result, int start)
{
    result.clear();
    if (start >= data.length()) {
        return data.length();
    }

    const int begin = data.indexOf('(', start);
    if (begin < 0) {
        return start;
    }

    result.reserve(16);

    int count = 0;
    int sublistBegin = start;
    bool insideQuote = false;
    for (int i = begin + 1; i < data.length(); ++i) {
        const char currentChar = data[i];
        if (currentChar == '(' && !insideQuote) {
            ++count;
            if (count == 1) {
                sublistBegin = i;
            }

            continue;
        }

        if (currentChar == ')' && !insideQuote) {
            if (count <= 0) {
                return i + 1;
            }

            if (count == 1) {
                result.append(data.mid(sublistBegin, i - sublistBegin + 1));
            }

            --count;
            continue;
        }

        if (currentChar == ' ' || currentChar == '\n' || currentChar == '\r') {
            continue;
        }

        if (count == 0) {
            QByteArray ba;
            const int consumed = ImapParser::parseString(data, ba, i);
            i = consumed - 1; // compensate for the for loop increment
            result.append(ba);
        } else if (count > 0) {
            if (currentChar == '"') {
                insideQuote = !insideQuote;
            } else if (currentChar == '\\' && insideQuote) {
                ++i;
                continue;
            }
        }
    }

    return data.length();
}

} // namespace

int ImapParser::parseParenthesizedList(const QByteArray &data, QVarLengthArray<QByteArray, 16> &result, int start)
{
    return parseParenthesizedListHelper(data, result, start);
}

int ImapParser::parseParenthesizedList(const QByteArray &data, QList<QByteArray> &result, int start)
{
    return parseParenthesizedListHelper(data, result, start);
}

int ImapParser::parseString(const QByteArray &data, QByteArray &result, int start)
{
    int begin = stripLeadingSpaces(data, start);
    result.clear();
    if (begin >= data.length()) {
        return data.length();
    }

    // literal string
    // TODO: error handling
    if (data[begin] == '{') {
        int end = data.indexOf('}', begin);
        Q_ASSERT(end > begin);
        int size = data.mid(begin + 1, end - begin - 1).toInt();

        // strip CRLF
        begin = end + 1;
        if (begin < data.length() && data[begin] == '\r') {
            ++begin;
        }
        if (begin < data.length() && data[begin] == '\n') {
            ++begin;
        }

        end = begin + size;
        result = data.mid(begin, end - begin);
        return end;
    }

    // quoted string
    return parseQuotedString(data, result, begin);
}

int ImapParser::parseQuotedString(const QByteArray &data, QByteArray &result, int start)
{
    int begin = stripLeadingSpaces(data, start);
    int end = begin;
    result.clear();
    if (begin >= data.length()) {
        return data.length();
    }

    bool foundSlash = false;
    // quoted string
    if (data[begin] == '"') {
        ++begin;
        result.reserve(qMin(32, data.size() - begin));
        for (int i = begin; i < data.length(); ++i) {
            const char ch = data.at(i);
            if (foundSlash) {
                foundSlash = false;
                if (ch == 'r') {
                    result += '\r';
                } else if (ch == 'n') {
                    result += '\n';
                } else if (ch == '\\') {
                    result += '\\';
                } else if (ch == '\"') {
                    result += '\"';
                } else {
                    //TODO: this is actually an error
                    result += ch;
                }
                continue;
            }
            if (ch == '\\') {
                foundSlash = true;
                continue;
            }
            if (ch == '"') {
                end = i + 1; // skip the '"'
                break;
            }
            result += ch;
        }
    } else {
        // unquoted string
        bool reachedInputEnd = true;
        for (int i = begin; i < data.length(); ++i) {
            const char ch = data.at(i);
            if (ch == ' ' || ch == '(' || ch == ')' || ch == '\n' || ch == '\r') {
                end = i;
                reachedInputEnd = false;
                break;
            }
            if (ch == '\\') {
                foundSlash = true;
            }
        }
        if (reachedInputEnd) {
            end = data.length();
        }
        result = data.mid(begin, end - begin);

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

    return end;
}

int ImapParser::stripLeadingSpaces(const QByteArray &data, int start)
{
    for (int i = start; i < data.length(); ++i) {
        if (data[i] != ' ') {
            return i;
        }
    }

    return data.length();
}

int ImapParser::parenthesesBalance(const QByteArray &data, int start)
{
    int count = 0;
    bool insideQuote = false;
    for (int i = start; i < data.length(); ++i) {
        const char ch = data[i];
        if (ch == '"') {
            insideQuote = !insideQuote;
            continue;
        }
        if (ch == '\\' && insideQuote) {
            ++i;
            continue;
        }
        if (ch == '(' && !insideQuote) {
            ++count;
            continue;
        }
        if (ch == ')' && !insideQuote) {
            --count;
            continue;
        }
    }
    return count;
}

QByteArray ImapParser::join(const QList<QByteArray> &list, const QByteArray &separator)
{
    // shortcuts for the easy cases
    if (list.isEmpty()) {
        return QByteArray();
    }
    if (list.size() == 1) {
        return list.first();
    }

    // avoid expensive realloc's by determining the size beforehand
    QList<QByteArray>::const_iterator it = list.constBegin();
    const QList<QByteArray>::const_iterator endIt = list.constEnd();
    int resultSize = (list.size() - 1) * separator.size();
    for (; it != endIt; ++it) {
        resultSize += (*it).size();
    }

    QByteArray result;
    result.reserve(resultSize);
    it = list.constBegin();
    result += (*it);
    ++it;
    for (; it != endIt; ++it) {
        result += separator;
        result += (*it);
    }

    return result;
}

QByteArray ImapParser::join(const QSet<QByteArray> &set, const QByteArray &separator)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    const QList<QByteArray> list = QList<QByteArray>::fromSet(set);
#else
    const QList<QByteArray> list(set.begin(), set.end());
#endif

    return ImapParser::join(list, separator);
}

int ImapParser::parseString(const QByteArray &data, QString &result, int start)
{
    QByteArray tmp;
    const int end = parseString(data, tmp, start);
    result = QString::fromUtf8(tmp);
    return end;
}

int ImapParser::parseNumber(const QByteArray &data, qint64 &result, bool *ok, int start)
{
    if (ok) {
        *ok = false;
    }

    int pos = stripLeadingSpaces(data, start);
    if (pos >= data.length()) {
        return data.length();
    }

    int begin = pos;
    for (; pos < data.length(); ++pos) {
        if (!isdigit(data.at(pos))) {
            break;
        }
    }

    const QByteArray tmp = data.mid(begin, pos - begin);
    result = tmp.toLongLong(ok);

    return pos;
}

QByteArray ImapParser::quote(const QByteArray &data)
{
    if (data.isEmpty()) {
        static const QByteArray empty("\"\"");
        return empty;
    }

    const int inputLength = data.length();
    int stuffToQuote = 0;
    for (int i = 0; i < inputLength; ++i) {
        const char ch = data.at(i);
        if (ch == '"' || ch == '\\' || ch == '\n' || ch == '\r') {
            ++stuffToQuote;
        }
    }

    QByteArray result;
    result.reserve(inputLength + stuffToQuote + 2);
    result += '"';

    // shortcut for the case that we don't need to quote anything at all
    if (stuffToQuote == 0) {
        result += data;
    } else {
        for (int i = 0; i < inputLength; ++i) {
            const char ch = data.at(i);
            if (ch == '\n') {
                result += "\\n";
                continue;
            }

            if (ch == '\r') {
                result += "\\r";
                continue;
            }

            if (ch == '"' || ch == '\\') {
                result += '\\';
            }

            result += ch;
        }
    }

    result += '"';
    return result;
}

int ImapParser::parseSequenceSet(const QByteArray &data, ImapSet &result, int start)
{
    int begin = stripLeadingSpaces(data, start);
    qint64 value = -1;
    qint64 lower = -1;
    qint64 upper = -1;
    for (int i = begin; i < data.length(); ++i) {
        if (data[i] == '*') {
            value = 0;
        } else if (data[i] == ':') {
            lower = value;
        } else if (isdigit(data[i])) {
            bool ok = false;
            i = parseNumber(data, value, &ok, i);
            Q_ASSERT(ok);   // TODO handle error
            --i;
        } else {
            upper = value;
            if (lower < 0) {
                lower = value;
            }
            result.add(ImapInterval(lower, upper));
            lower = -1;
            upper = -1; // NOLINT(clang-analyzer-deadcode.DeadStores) // false positive?
            value = -1;
            if (data[i] != ',') {
                return i;
            }
        }
    }
    // take care of left-overs at input end
    upper = value;
    if (lower < 0) {
        lower = value;
    }

    if (lower >= 0 && upper >= 0) {
        result.add(ImapInterval(lower, upper));
    }

    return data.length();
}

int ImapParser::parseDateTime(const QByteArray &data, QDateTime &dateTime, int start)
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

    int pos = stripLeadingSpaces(data, start);
    if (data.length() <= pos) {
        return pos;
    }

    bool quoted = false;
    if (data[pos] == '"') {
        quoted = true;
        ++pos;

        if (data.length() <= pos + 26) {
            return start;
        }
    } else {
        if (data.length() < pos + 26) {
            return start;
        }
    }

    bool ok = true;
    const int day = (data[pos] == ' ' ? data[pos + 1] - '0'  // single digit day
                     : data.mid(pos, 2).toInt(&ok));
    if (!ok) {
        return start;
    }

    pos += 3;
    static const QByteArray shortMonthNames("janfebmaraprmayjunjulaugsepoctnovdec");
    int month = shortMonthNames.indexOf(data.mid(pos, 3).toLower());
    if (month == -1) {
        return start;
    }

    month = month / 3 + 1;
    pos += 4;
    const int year = data.mid(pos, 4).toInt(&ok);
    if (!ok) {
        return start;
    }

    pos += 5;
    const int hours = data.mid(pos, 2).toInt(&ok);
    if (!ok) {
        return start;
    }

    pos += 3;
    const int minutes = data.mid(pos, 2).toInt(&ok);
    if (!ok) {
        return start;
    }

    pos += 3;
    const int seconds = data.mid(pos, 2).toInt(&ok);
    if (!ok) {
        return start;
    }

    pos += 4;
    const int tzhh = data.mid(pos, 2).toInt(&ok);
    if (!ok) {
        return start;
    }

    pos += 2;
    const int tzmm = data.mid(pos, 2).toInt(&ok);
    if (!ok) {
        return start;
    }

    int tzsecs = tzhh * 60 * 60 + tzmm * 60;
    if (data[pos - 3] == '-') {
        tzsecs = -tzsecs;
    }

    const QDate date(year, month, day);
    const QTime time(hours, minutes, seconds);
    dateTime = QDateTime(date, time, Qt::UTC);
    if (!dateTime.isValid()) {
        return start;
    }

    dateTime = dateTime.addSecs(-tzsecs);

    pos += 2;
    if (data.length() <= pos || !quoted) {
        return pos;
    }

    if (data[pos] == '"') {
        ++pos;
    }

    return pos;
}

void ImapParser::splitVersionedKey(const QByteArray &data, QByteArray &key, int &version)
{
    const int startPos = data.indexOf('[');
    const int endPos = data.indexOf(']');
    if (startPos != -1 && endPos != -1) {
        if (endPos > startPos) {
            bool ok = false;

            version = data.mid(startPos + 1, endPos - startPos - 1).toInt(&ok);
            if (!ok) {
                version = 0;
            }

            key = data.left(startPos);
        }
    } else {
        key = data;
        version = 0;
    }
}

ImapParser::ImapParser()
    : d(new Private)
{
    reset();
}

ImapParser::~ImapParser()
{
    delete d;
}

bool ImapParser::parseNextLine(const QByteArray &readBuffer)
{
    d->continuation = false;

    // first line, get the tag
    if (d->tagBuffer.isEmpty()) {
        const int startOfData = ImapParser::parseString(readBuffer, d->tagBuffer);
        if (startOfData < readBuffer.length() && startOfData >= 0) {
            d->dataBuffer = readBuffer.mid(startOfData + 1);
        }

    } else {
        d->dataBuffer += readBuffer;
    }

    // literal read in progress
    if (d->literalSize > 0) {
        d->literalSize -= readBuffer.size();

        // still not everything read
        if (d->literalSize > 0) {
            return false;
        }

        // check the remaining (non-literal) part for parentheses
        if (d->literalSize < 0) {
            // the following looks strange but works since literalSize can be negative here
            d->parenthesesCount += ImapParser::parenthesesBalance(readBuffer, readBuffer.length() + static_cast<int>(d->literalSize));

            // check if another literal read was started
            if (d->checkLiteralStart(readBuffer, readBuffer.length() + static_cast<int>(d->literalSize))) {
                return false;
            }
        }

        // literal string finished but still open parentheses
        if (d->parenthesesCount > 0) {
            return false;
        }

    } else {

        // open parentheses
        d->parenthesesCount += ImapParser::parenthesesBalance(readBuffer);

        // start new literal read
        if (d->checkLiteralStart(readBuffer)) {
            return false;
        }

        // still open parentheses
        if (d->parenthesesCount > 0) {
            return false;
        }

        // just a normal response, fall through
    }

    return true;
}

void ImapParser::parseBlock(const QByteArray &data)
{
    Q_ASSERT(d->literalSize >= data.size());
    d->literalSize -= data.size();
    d->dataBuffer += data;
}

QByteArray ImapParser::tag() const
{
    return d->tagBuffer;
}

QByteArray ImapParser::data() const
{
    return d->dataBuffer;
}

void ImapParser::reset()
{
    d->dataBuffer.clear();
    d->tagBuffer.clear();
    d->parenthesesCount = 0;
    d->literalSize = 0;
    d->continuation = false;
}

bool ImapParser::continuationStarted() const
{
    return d->continuation;
}

qint64 ImapParser::continuationSize() const
{
    return d->literalSize;
}
