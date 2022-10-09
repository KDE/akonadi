/*
    SPDX-FileCopyrightText: 2019 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "dbexception.h"

namespace Akonadi
{
namespace Server
{
/**
  This class catches DbDeadlockException (as emitted by QueryBuilder)
  and retries execution of the method when it happens, as required by
  SQL databases.
*/
class DbDeadlockCatcher
{
public:
    template<typename Func>
    explicit DbDeadlockCatcher(Func &&func)
    {
        callFunc(func, 0);
    }

private:
    static const int MaxRecursion = 5;
    template<typename Func>
    void callFunc(Func &&func, int recursionCounter)
    {
        try {
            func();
        } catch (const DbDeadlockException &) {
            if (recursionCounter == MaxRecursion) {
                throw;
            } else {
                callFunc(func, ++recursionCounter); // recurse
            }
        }
    }
};

} // namespace Server
} // namespace Akonadi
