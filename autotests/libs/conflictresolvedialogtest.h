/*
    SPDX-FileCopyrightText: 2017-2022 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class ConflictResolveDialogTest : public QObject
{
    Q_OBJECT
public:
    explicit ConflictResolveDialogTest(QObject *parent = nullptr);
    ~ConflictResolveDialogTest() override = default;

private Q_SLOTS:
    void shouldHaveDefaultValues();
};
