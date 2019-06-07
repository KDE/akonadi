/*
    Copyright (C) 2019  Daniel Vrátil <dvratil@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
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
