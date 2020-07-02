/*
    SPDX-FileCopyrightText: 2019 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_AKSCOPEGUARD_H_
#define AKONADI_AKSCOPEGUARD_H_

#include <functional>
#include <type_traits>

namespace Akonadi {

class AkScopeGuard
{
public:
   template<typename U>
    AkScopeGuard(U &&fun)
        : mFun(std::move(fun))
    {}

    AkScopeGuard(const AkScopeGuard &) = delete;
    AkScopeGuard(AkScopeGuard &&) = default;
    AkScopeGuard &operator=(const AkScopeGuard &) = delete;
    AkScopeGuard &operator=(AkScopeGuard &&) = delete;

    ~AkScopeGuard()
    {
        mFun();
    }

private:
    std::function<void()> mFun;
};

} // namespace Akonadi

#endif
