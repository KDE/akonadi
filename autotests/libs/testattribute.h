/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "attribute.h"

#include <QByteArray>

/* Attribute used for testing by various unit tests. */
class TestAttribute : public Akonadi::Attribute
{
public:
    TestAttribute()
    {
    }
    QByteArray type() const override
    {
        return "EXTRA";
    }
    QByteArray serialized() const override
    {
        return data;
    }
    void deserialize(const QByteArray &ba) override
    {
        data = ba;
    }
    TestAttribute *clone() const override
    {
        auto a = new TestAttribute;
        a->data = data;
        return a;
    }
    QByteArray data;
};
