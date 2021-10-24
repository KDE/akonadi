/*
    SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class CachePolicyTest : public QObject
{
    Q_OBJECT
public:
    explicit CachePolicyTest(QObject *parent = nullptr);
    ~CachePolicyTest() override;
private Q_SLOTS:
    void shouldHaveDefaultValue();
};

