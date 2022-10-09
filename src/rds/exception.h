/***************************************************************************
 *   SPDX-FileCopyrightText: 2010 Marc Mutz <mutz@kde.org>                 *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/
#pragma once

#include <QString>
#include <stdexcept>

template<typename Ex>
class Exception : Ex
{
public:
    explicit Exception(const QString &message)
        : Ex(message.toStdString())
    {
    }

    ~Exception() throw() override
    {
    }
};
