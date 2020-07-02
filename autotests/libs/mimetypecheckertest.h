/*
    SPDX-FileCopyrightText: 2009 Kevin Krammer <kevin.krammer@gmx.at>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_MIMETYPECHECKERTEST_H
#define AKONADI_MIMETYPECHECKERTEST_H

#include "mimetypechecker.h"

#include <QObject>
#include <QStringList>

class MimeTypeCheckerTest : public QObject
{
    Q_OBJECT
public:
    explicit MimeTypeCheckerTest(QObject *parent = nullptr);

private:
    QStringList mCalendarSubTypes;

    Akonadi::MimeTypeChecker mEmptyChecker;
    Akonadi::MimeTypeChecker mCalendarChecker;
    Akonadi::MimeTypeChecker mSubTypeChecker;
    Akonadi::MimeTypeChecker mAliasChecker;

private Q_SLOTS:
    void initTestCase();
    void testCollectionCheck();
    void testItemCheck();
    void testStringMatchEquivalent();
};

#endif
