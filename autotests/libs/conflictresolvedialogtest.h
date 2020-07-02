/*
    SPDX-FileCopyrightText: 2017-2020 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CONFLICTRESOLVEDIALOGTEST_H
#define CONFLICTRESOLVEDIALOGTEST_H

#include <QObject>

class ConflictResolveDialogTest : public QObject
{
    Q_OBJECT
public:
    explicit ConflictResolveDialogTest(QObject *parent = nullptr);
    ~ConflictResolveDialogTest() = default;

private Q_SLOTS:
    void shouldHaveDefaultValues();
};

#endif // CONFLICTRESOLVEDIALOGTEST_H
