/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class ImapParserTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testStripLeadingSpaces();
    void testParseQuotedString();
    void testParseString();
    void testParseParenthesizedList_data();
    void testParseParenthesizedList();
    void testParseNumber();
    void testQuote_data();
    void testQuote();
    void testMessageParser_data();
    void testMessageParser();
    void testParseSequenceSet_data();
    void testParseSequenceSet();
    void testParseDateTime_data();
    void testParseDateTime();
    void testBulkParser_data();
    void testBulkParser();
    void testJoin_data();
    void testJoin();

    void benchParseQuotedString_data();
    void benchParseQuotedString();
};

